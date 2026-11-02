/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */

#include <stdio.h>
#include <string.h>
#include "tim.h"
#include "proto_transport.h"

uint8_t g_usart2_rx_sta;           

uint8_t g_usart2_rx_buffer[RXBUFFERSIZE];   

uint8_t uart_dma_busy = 0;

/* UART3 DMA RX/TX variables */
uint8_t  g_usart3_dma_rx_buf[UART3_DMA_RX_BUF_SIZE];
volatile uint8_t  g_usart3_dma_rx_ready = 0U;
volatile uint16_t g_usart3_dma_rx_len = 0U;
volatile uint8_t  g_usart3_dma_tx_busy = 0U;
volatile uint32_t g_usart3_dma_rx_count = 0U;

static volatile uint8_t g_usart2_debug_echo_enabled = 1U;
static volatile uint8_t g_usart2_debug_rx_pending = 0U;
static volatile uint8_t g_usart2_debug_last_rx_byte = 0U;
static volatile uint32_t g_usart2_debug_rx_count = 0U;
static volatile uint32_t g_usart2_debug_last_tx_tick = 0U;

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 230400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  
    /* USART1 is used as a plain printf/debug TX port only. */
    printf("USART1 ready: 230400 bps\r\n");
  
  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 230400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  
  HAL_UART_Receive_IT(&huart2,(uint8_t *)g_usart2_rx_buffer,RXBUFFERSIZE);
  USART2_DebugSendString("USART2 ready: 230400 bps\r\n");

  /* USER CODE END USART2_Init 2 */

}
/* USART3 init function */

uint8_t UART3_SendDMA(uint8_t *pData, uint16_t Size)
{
    if ((pData == NULL) || (Size == 0U))
    {
        return 0U;
    }

    if (g_usart3_dma_tx_busy)
    {
        return 0U;
    }

    g_usart3_dma_tx_busy = 1U;
    if (HAL_UART_Transmit_DMA(&huart3, pData, Size) != HAL_OK)
    {
        g_usart3_dma_tx_busy = 0U;
        return 0U;
    }

    return 1U;
}

void USART3_SendString(const char *msg)
{
    uint16_t len;

    if ((msg == NULL) || (huart3.Instance == NULL))
    {
        return;
    }

    len = (uint16_t)strlen(msg);
    if (len == 0U)
    {
        return;
    }

    {
        uint32_t timeout = HAL_GetTick() + 100U;
        while (g_usart3_dma_tx_busy && (HAL_GetTick() < timeout)) {}
    }

    (void)UART3_SendDMA((uint8_t *)msg, len);
}

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 230400;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
  /* Start DMA RX (no boot banner — keep UART3 protocol-clean) */
  (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart3, g_usart3_dma_rx_buf, UART3_DMA_RX_BUF_SIZE);
  /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = UART1_TX_Pin|UART1_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */
  /*
   * Board route note:
   * USART2 is wired to PD5(TX) / PD6(RX) on this PCB, not PA2 / PA3.
   * If CubeMX regenerates this file, keep the USART2 GPIO block on PD5 / PD6.
   */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PD5     ------> USART2_TX
    PD6     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = UART2_TX_Pin|UART2_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART3 DMA Init */
    /* USART3_RX Init */
    hdma_usart3_rx.Instance = DMA1_Stream1;
    hdma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode = DMA_NORMAL;
    hdma_usart3_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart3_rx);

    /* USART3_TX Init */
    hdma_usart3_tx.Instance = DMA1_Stream3;
    hdma_usart3_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;
    hdma_usart3_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart3_tx);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, UART1_TX_Pin|UART1_RX_Pin);

  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PD5     ------> USART2_TX
    PD6     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOD, UART2_TX_Pin|UART2_RX_Pin);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);

    /* USART3 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
  * @brief USART2�����жϻص�����
  * @param huart: UART���ָ��?
  */
// 协议帧定�?
#define PROTOCOL_HEAD1 0x6D
#define PROTOCOL_HEAD2 0x6A
#define USART2_PROTOCOL_DEBUG 0
#define USART2_RAW_DEBUG 0
#define USART2_DEBUG_RX_LOG_SIZE 64
#define PROTOCOL_LENGTH 14  // 完整帧长度：2字节�?+ 1字节地址 + 2字节命令 + 8字节数据 + 1字节校验

// 接收状态机
typedef enum {
    RX_STATE_WAIT_HEAD1,
    RX_STATE_WAIT_HEAD2,
    RX_STATE_WAIT_ADDR,
    RX_STATE_WAIT_CMD1,
    RX_STATE_WAIT_CMD2,
    RX_STATE_WAIT_DATA1,
    RX_STATE_WAIT_DATA2,
    RX_STATE_WAIT_DATA3,
    RX_STATE_WAIT_DATA4,
    RX_STATE_WAIT_DATA5,
    RX_STATE_WAIT_DATA6,
    RX_STATE_WAIT_DATA7,
    RX_STATE_WAIT_DATA8,
    RX_STATE_WAIT_CHECKSUM
} RxState;

// 全局变量
RxState g_rx_state = RX_STATE_WAIT_HEAD1;
uint8_t g_rx_buffer[PROTOCOL_LENGTH];
uint8_t g_rx_index = 0;
#if USART2_RAW_DEBUG
static volatile uint8_t g_usart2_debug_rx_log[USART2_DEBUG_RX_LOG_SIZE];
static volatile uint16_t g_usart2_debug_rx_head = 0;
static volatile uint16_t g_usart2_debug_rx_tail = 0;
static volatile uint8_t g_usart2_debug_rx_overflow = 0;
#endif
#define DR304_MODBUS_ADDR      0x01U
#define DR304_MODBUS_FUNC      0x03U
#define DR304_BYTE_COUNT       24U
#define DR304_CHANNEL_COUNT    6U
#define DR304_FRAME_LENGTH     (3U + DR304_BYTE_COUNT + 2U)

typedef enum {
    DR304_RX_STATE_WAIT_ADDR,
    DR304_RX_STATE_WAIT_FUNC,
    DR304_RX_STATE_WAIT_BYTE_COUNT,
    DR304_RX_STATE_READ_BODY
} DR304_RxState;

static DR304_RxState g_dr304_rx_state = DR304_RX_STATE_WAIT_ADDR;
static uint8_t g_dr304_rx_buffer[DR304_FRAME_LENGTH];
static uint8_t g_dr304_rx_index = 0U;
static volatile int32_t g_dr304_latest_value[DR304_CHANNEL_COUNT];
static volatile uint8_t g_dr304_frame_ready = 0U;
static volatile uint32_t g_dr304_frame_count = 0U;
static volatile uint32_t g_dr304_crc_error_count = 0U;

static void USART2_DR304ResetParser(void)
{
    g_dr304_rx_state = DR304_RX_STATE_WAIT_ADDR;
    g_dr304_rx_index = 0U;
}

static uint16_t USART2_ModbusCRC16(const uint8_t *buffer, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;

    while(len-- != 0U)
    {
        crc ^= *buffer++;
        for(i = 0U; i < 8U; i++)
        {
            if((crc & 0x0001U) != 0U)
            {
                crc >>= 1U;
                crc ^= 0xA001U;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

static int32_t USART2_DR304ReadInt32BE(const uint8_t *data)
{
    uint32_t raw;

    raw = ((uint32_t)data[0] << 24) |
          ((uint32_t)data[1] << 16) |
          ((uint32_t)data[2] << 8) |
          ((uint32_t)data[3]);

    return (int32_t)raw;
}

static void USART2_DR304StoreFrame(const uint8_t *frame)
{
    uint16_t crc_calc;
    uint16_t crc_rx;
    uint8_t i;

    crc_calc = USART2_ModbusCRC16(frame, (uint16_t)(DR304_FRAME_LENGTH - 2U));
    crc_rx = (uint16_t)frame[DR304_FRAME_LENGTH - 2U] |
             ((uint16_t)frame[DR304_FRAME_LENGTH - 1U] << 8);

    if(crc_calc != crc_rx)
    {
        g_dr304_crc_error_count++;
        return;
    }

    for(i = 0U; i < DR304_CHANNEL_COUNT; i++)
    {
        g_dr304_latest_value[i] = USART2_DR304ReadInt32BE(&frame[3U + (i * 4U)]);
    }

    g_dr304_frame_count++;
    g_dr304_frame_ready = 1U;
}

static uint8_t USART2_DR304StartCandidate(uint8_t byte)
{
    if(byte != DR304_MODBUS_ADDR)
    {
        return 0U;
    }

    g_dr304_rx_buffer[0] = byte;
    g_dr304_rx_index = 1U;
    g_dr304_rx_state = DR304_RX_STATE_WAIT_FUNC;
    return 1U;
}

static uint8_t USART2_DR304ProcessByte(uint8_t byte)
{
    switch(g_dr304_rx_state)
    {
        case DR304_RX_STATE_WAIT_ADDR:
            return USART2_DR304StartCandidate(byte);

        case DR304_RX_STATE_WAIT_FUNC:
            if(byte != DR304_MODBUS_FUNC)
            {
                USART2_DR304ResetParser();
                return USART2_DR304StartCandidate(byte);
            }
            g_dr304_rx_buffer[g_dr304_rx_index++] = byte;
            g_dr304_rx_state = DR304_RX_STATE_WAIT_BYTE_COUNT;
            return 1U;

        case DR304_RX_STATE_WAIT_BYTE_COUNT:
            if(byte != DR304_BYTE_COUNT)
            {
                USART2_DR304ResetParser();
                return USART2_DR304StartCandidate(byte);
            }
            g_dr304_rx_buffer[g_dr304_rx_index++] = byte;
            g_dr304_rx_state = DR304_RX_STATE_READ_BODY;
            return 1U;

        case DR304_RX_STATE_READ_BODY:
            g_dr304_rx_buffer[g_dr304_rx_index++] = byte;
            if(g_dr304_rx_index >= DR304_FRAME_LENGTH)
            {
                USART2_DR304StoreFrame(g_dr304_rx_buffer);
                USART2_DR304ResetParser();
            }
            return 1U;

        default:
            USART2_DR304ResetParser();
            break;
    }

    return 0U;
}

void USART2_DR304PrintTask(void)
{
    int32_t value[DR304_CHANNEL_COUNT] = {0};
    uint32_t frame_count = 0U;
    uint32_t crc_error_count = 0U;
    uint32_t primask;
    uint8_t has_frame;
    uint8_t i;

    primask = __get_PRIMASK();
    __disable_irq();

    has_frame = g_dr304_frame_ready;
    if(has_frame != 0U)
    {
        for(i = 0U; i < DR304_CHANNEL_COUNT; i++)
        {
            value[i] = g_dr304_latest_value[i];
        }
        frame_count = g_dr304_frame_count;
        crc_error_count = g_dr304_crc_error_count;
        g_dr304_frame_ready = 0U;
    }

    if(primask == 0U)
    {
        __enable_irq();
    }

    if(has_frame == 0U)
    {
        return;
    }

    printf("[DR304 #%lu] Fx=%.2f Fy=%.2f Fz=%.2f Mx=%.2f My=%.2f Mz=%.2f crc_err=%lu\r\n",
           (unsigned long)frame_count,
           value[0]/100.0f,
           value[1]/100.0f,
           value[2]/100.0f,
           value[3]/100.0f,
           value[4]/100.0f,
           value[5]/100.0f,
           (unsigned long)crc_error_count);
}

static float g_force_zero_offset[DR304_CHANNEL_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

void USART2_DR304GetLatestValues(float values[6])
{
	uint32_t primask;
	uint8_t i;

	primask = __get_PRIMASK();
	__disable_irq();

	for(i = 0U; i < DR304_CHANNEL_COUNT; i++)
	{
		values[i] = (float)g_dr304_latest_value[i] * 0.01f - g_force_zero_offset[i];
	}

	if(primask == 0U)
	{
		__enable_irq();
	}
}

void USART2_DR304SetZero(void)
{
	uint32_t primask;
	uint8_t i;
	int32_t raw[DR304_CHANNEL_COUNT];

	primask = __get_PRIMASK();
	__disable_irq();

	for(i = 0U; i < DR304_CHANNEL_COUNT; i++)
	{
		raw[i] = g_dr304_latest_value[i];
	}

	if(primask == 0U)
	{
		__enable_irq();
	}

	/* Accumulate: new offset = current raw value (as float) + previous offset */
	for(i = 0U; i < DR304_CHANNEL_COUNT; i++)
	{
		g_force_zero_offset[i] = (float)raw[i] * 0.01f;
	}

	printf("[DR304] zero set: Fx=%.2f Fy=%.2f Fz=%.2f Mx=%.2f My=%.2f Mz=%.2f\r\n",
	       (double)g_force_zero_offset[0], (double)g_force_zero_offset[1],
	       (double)g_force_zero_offset[2], (double)g_force_zero_offset[3],
	       (double)g_force_zero_offset[4], (double)g_force_zero_offset[5]);
}

static void USART2_DebugLogByte(uint8_t byte)
{
#if USART2_RAW_DEBUG
	uint16_t next_head = (uint16_t)((g_usart2_debug_rx_head + 1U) % USART2_DEBUG_RX_LOG_SIZE);
	if(next_head == g_usart2_debug_rx_tail)
	{
		g_usart2_debug_rx_overflow = 1;
		return;
	}
	g_usart2_debug_rx_log[g_usart2_debug_rx_head] = byte;
	g_usart2_debug_rx_head = next_head;
#else
	(void)byte;
#endif
}

void USART2_DebugSendString(const char *msg)
{
    uint16_t len;

    if ((msg == NULL) || (huart1.Instance == NULL))
    {
        return;
    }

    len = (uint16_t)strlen(msg);
    if (len == 0U)
    {
        return;
    }

    (void)HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, 50U);
}

void USART2_DebugPoll(void)
{
    if(g_usart2_debug_rx_pending != 0U)
    {
        uint8_t tx_byte = g_usart2_debug_last_rx_byte;
        g_usart2_debug_rx_pending = 0U;
        if(g_usart2_debug_echo_enabled != 0U)
        {
            (void)HAL_UART_Transmit(&huart2, &tx_byte, 1U, 5U);
        }
    }

    if((uint32_t)(HAL_GetTick() - g_usart2_debug_last_tx_tick) >= 1000U)
    {
        g_usart2_debug_last_tx_tick = HAL_GetTick();
        USART2_DebugSendString("USART2 dbg: waiting for data\r\n");
    }
}

static void USART2_PrintFrame(const char *tag, const uint8_t *frame, uint16_t len)
{
#if USART2_PROTOCOL_DEBUG
	uint16_t i;
	printf("%s", tag);
	for(i = 0; i < len; i++)
	{
		printf(" %02X", frame[i]);
	}
	printf("\r\n");
#else
	(void)tag;
	(void)frame;
	(void)len;
#endif
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        uart_dma_busy = 0;
    }
    else if(huart->Instance == USART3)
    {
        g_usart3_dma_tx_busy = 0;
    }
}

// 定义存放上位机解析结果的结构�?
typedef struct {
    float position_ref;
    float velocity_ref;
    float torque_ref;
    float kp;
    float kd;
} RS00_TargetParamsTypeDef;

// 解析上位�?8 字节混合数据，转换为 RS00 电机的浮点数
RS00_TargetParamsTypeDef parse_host_dynamic_params(uint8_t data_bytes[8])
{
    RS00_TargetParamsTypeDef m_para;
    // 1. 解析目标角度 (-12.57 ~ 12.57)
    uint16_t angle_uint = ((uint16_t)data_bytes[0] << 8) | data_bytes[1];
    m_para.position_ref = ((float)angle_uint / 65535.0f) * 25.14f - 12.57f;

    // 2. 解析目标速度 (-33.0 ~ 33.0)
    uint16_t velocity_uint = ((uint16_t)data_bytes[2] << 4) | (data_bytes[3] >> 4);
    m_para.velocity_ref = ((float)velocity_uint / 4095.0f) * 66.0f - 33.0f;

    // 3. 解析 Kp (0 ~ 500)
    uint16_t kp_uint = ((uint16_t)(data_bytes[3] & 0x0F) << 8) | data_bytes[4];
    m_para.kp = ((float)kp_uint / 4095.0f) * 500.0f;

    // 4. 解析 Kd (0 ~ 5)
    uint16_t kd_uint = ((uint16_t)data_bytes[5] << 4) | (data_bytes[6] >> 4);
    m_para.kd = ((float)kd_uint / 4095.0f) * 5.0f;

    // 5. 解析目标力矩 (-14.0 ~ 14.0)
    uint16_t torque_uint = ((uint16_t)(data_bytes[6] & 0x0F) << 8) | data_bytes[7];
    m_para.torque_ref = ((float)torque_uint / 4095.0f) * 28.0f - 14.0f;

    return m_para;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART3)
	{
		/* DMA buffer full: feed all bytes to transport parser */
		g_usart3_dma_rx_count += UART3_DMA_RX_BUF_SIZE;
		proto_transport_rx_process(g_usart3_dma_rx_buf, UART3_DMA_RX_BUF_SIZE);
		(void)HAL_UARTEx_ReceiveToIdle_DMA(&huart3, g_usart3_dma_rx_buf, UART3_DMA_RX_BUF_SIZE);
		return;
	}

	if(huart->Instance == USART2)
	{
		uint8_t uc_rx = g_usart2_rx_buffer[0];
		uint8_t checksum = 0;
		g_usart2_debug_last_rx_byte = uc_rx;
		g_usart2_debug_rx_pending = 1U;
		g_usart2_debug_rx_count++;
		int i;
		USART2_DebugLogByte(uc_rx);
		if(((g_rx_state == RX_STATE_WAIT_HEAD1) ||
		    (g_dr304_rx_state != DR304_RX_STATE_WAIT_ADDR) ||
		    ((g_rx_state == RX_STATE_WAIT_HEAD2) && (uc_rx != PROTOCOL_HEAD2))) &&
		   (USART2_DR304ProcessByte(uc_rx) != 0U))
		{
			g_rx_state = RX_STATE_WAIT_HEAD1;
			HAL_UART_Receive_IT(&huart2, (uint8_t *)g_usart2_rx_buffer, RXBUFFERSIZE);
			return;
		}

// 状态机处理
		switch(g_rx_state)
		{
			case RX_STATE_WAIT_HEAD1:
				if(uc_rx == PROTOCOL_HEAD1)
				{
					g_rx_buffer[0] = uc_rx;
					g_rx_state = RX_STATE_WAIT_HEAD2;
				}
				break;
				
			case RX_STATE_WAIT_HEAD2:
				if(uc_rx == PROTOCOL_HEAD2)
				{
					g_rx_buffer[1] = uc_rx;
					g_rx_state = RX_STATE_WAIT_ADDR;
				}
				else if(uc_rx == PROTOCOL_HEAD1)
				{
					// 可能是前一帧不完整，重新开�?
					g_rx_buffer[0] = uc_rx;
					g_rx_state = RX_STATE_WAIT_HEAD2;
				}
				else
				{
					g_rx_state = RX_STATE_WAIT_HEAD1;
				}
				break;
				
			case RX_STATE_WAIT_ADDR:
				if(uc_rx == 0x01)  // 只处理地址0x01
				{
					g_rx_buffer[2] = uc_rx;
					g_rx_state = RX_STATE_WAIT_CMD1;
				}
				else
				{
					g_rx_state = RX_STATE_WAIT_HEAD1;
				}
				break;
				
			case RX_STATE_WAIT_CMD1:
				g_rx_buffer[3] = uc_rx;
				g_rx_state = RX_STATE_WAIT_CMD2;
				break;
				
			case RX_STATE_WAIT_CMD2:
				g_rx_buffer[4] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA1;
				break;
				
			case RX_STATE_WAIT_DATA1:
				g_rx_buffer[5] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA2;
				break;
				
			case RX_STATE_WAIT_DATA2:
				g_rx_buffer[6] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA3;
				break;
				
			case RX_STATE_WAIT_DATA3:
				g_rx_buffer[7] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA4;
				break;
				
			case RX_STATE_WAIT_DATA4:
				g_rx_buffer[8] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA5;
				break;
				
			case RX_STATE_WAIT_DATA5:
				g_rx_buffer[9] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA6;
				break;
				
			case RX_STATE_WAIT_DATA6:
				g_rx_buffer[10] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA7;
				break;
				
			case RX_STATE_WAIT_DATA7:
				g_rx_buffer[11] = uc_rx;
				g_rx_state = RX_STATE_WAIT_DATA8;
				break;
				
			case RX_STATE_WAIT_DATA8:
				g_rx_buffer[12] = uc_rx;
				g_rx_state = RX_STATE_WAIT_CHECKSUM;
				break;
				
			case RX_STATE_WAIT_CHECKSUM:
				g_rx_buffer[13] = uc_rx;
				
				// 计算校验�?
				checksum = 0;
				for(i = 2; i < 13; i++)  // 地址到数�?的和
				{
					checksum += g_rx_buffer[i];
				}
				
				// 校验和正�?
				if(checksum == g_rx_buffer[13])
				{
					USART2_PrintFrame("[USART2] frame OK:", g_rx_buffer, PROTOCOL_LENGTH);
					// 解析命令
					uint8_t cmd1 = g_rx_buffer[3];
					uint8_t cmd2 = g_rx_buffer[4];
					
					uint8_t data_bytes[4] = {g_rx_buffer[5], g_rx_buffer[6], g_rx_buffer[7], g_rx_buffer[8]};
					uint8_t data_bytes_ext[4] = {g_rx_buffer[9], g_rx_buffer[10], g_rx_buffer[11], g_rx_buffer[12]};

					uint8_t data_whole[8] = {g_rx_buffer[5], g_rx_buffer[6], g_rx_buffer[7], g_rx_buffer[8], g_rx_buffer[9], g_rx_buffer[10], g_rx_buffer[11], g_rx_buffer[12]};
					
					float data_float;
					float data_float_ext;

					
					// 根据命令类型处理
					if(cmd1 == 0x01)  // 电机1整数命令
					{
						printf("cmd 2 = %d \r\n",cmd2);
						if(cmd2 == 0x01)
						{
							// 启动/停止命令 - 只使用第5字节和第6字节
							uint8_t data1 = g_rx_buffer[5];  // �?字节

							// 根据协议�?x01 0x00表示启动�?x00 0x00表示停止
							if(data1 == 0x01)
							{
								my_Order_mode.start_or_stop = _start;
								TIM2_Start_IT();			
								My_state = stop_flag;
								printf("start \r\n");
							}
							else if(data1 == 0x00)
							{
								my_Order_mode.start_or_stop = _stop;
								TIM2_Stop_IT();				
								My_state = stop_flag;
								printf("stop \r\n");
							}
								
						}
						// 其他整数命令暂不处理
					}
					else if(cmd1 == 0x06)  // 电机1浮点命令
					{
						// 直接使用memcpy转换，上位机已按嵌入式存储方式发�?
						memcpy(&data_float, data_bytes, 4);
						memcpy(&data_float_ext, data_bytes_ext, 4);

						switch(cmd2)
						{
							case 0x01:  // 角度指令
								my_Order_mode.motor1_jiaodu = data_float;
								printf("my_Order_mode.motor1_jiaodu = %.4f \r\n" ,my_Order_mode.motor1_jiaodu);
								break;
							case 0x02:  // 速度指令
								my_Order_mode.motor1_sudu = data_float;
								printf("my_Order_mode.motor1_sudu = %.4f \r\n" ,my_Order_mode.motor1_sudu);
								break;
							case 0x03:  // 电流指令
								my_Order_mode.motor1_dianliu = data_float;
								printf("my_Order_mode.motor1_dianliu = %.4f \r\n" ,my_Order_mode.motor1_dianliu);
								break;
							case 0x04:  // kp
								my_Order_mode.motor1_kp = data_float;
								Control_P_PID.kp = my_Order_mode.motor1_kp;
								printf("my_Order_mode.motor1_kp = %.4f \r\n" ,my_Order_mode.motor1_kp);
								break;
							case 0x05:  // ki
								my_Order_mode.motor1_ki = data_float;
								Control_P_PID.ki = my_Order_mode.motor1_ki;
								printf("my_Order_mode.motor1_ki = %.4f \r\n" ,my_Order_mode.motor1_ki);
								break;
							// 新增命令可使用data_float_ext
							default:
								break;
						}
					}
					else if(cmd1 == 0x11)  // 电机2整数命令 (RS00 控制)
					{
						printf("cmd 2 = %d \r\n",cmd2);
						uint8_t data1 = g_rx_buffer[5];  // �?字节
						switch (cmd2)
						{
							case 0x01: // 启动/停止命令
								if(data1 == 0x01)
								{
									RS00_Enable(0x7F, 0xFD);
									printf("RS00 Motor Enabled \r\n");
								}
								else if(data1 == 0x00)
								{
									RS00_Stop(0x7F, 0xFD);
									printf("RS00 Motor Stopped \r\n");
								}	
							    break;
							case 0x02: // 设置运行模式
							    // data1 = 1(运控), 2(位置), 3(速度), 5(参数读写)
								RS00_SetRunMode(0x7F, 0xFD, data1);
								printf("RS00 Mode Selected: %d \r\n", data1);
								break;
							case 0x03: // 设置机械零点
								RS00_SetZeroPosition(0x7F, 0xFD);
								printf("RS00 Zero Position Set \r\n");
								break;
							case 0x04: // 清除错误/复位 (私有协议直接重启使能即可)
							    RS00_StopAndClearError(0x7F, 0xFD);   // 发送带清故障标志的停止�?
								HAL_Delay(50);
								RS00_Enable(0x7F, 0xFD);             // 重新使能
								printf("RS00 Error Cleared (Restarted) \r\n");
								break;							
							default:
								break;
						}
					}
					else if(cmd1 == 0x16)  // 电机2浮点命令 (RS00 高级控制)
					{
						printf("motor2 float\r\n");
						// 直接使用memcpy转换，上位机已按嵌入式存储方式发�?
						memcpy(&data_float, data_bytes, 4);
						memcpy(&data_float_ext, data_bytes_ext, 4);

						printf("data_float = %f, data_float_ext = %f\r\n", data_float, data_float_ext);
						printf("cmd2 = %d\r\n", cmd2);

						switch(cmd2)
						{
						case 0x01:  // �?RS00 运控指令下发（多参数组合）★
								{
								// 解析8字节混合数据
								RS00_TargetParamsTypeDef my_para = parse_host_dynamic_params(data_whole);

									// 发送运控指令（通信类型1�?
								RS00_Control(0x7F, 0xFD,
											my_para.position_ref,
											my_para.velocity_ref,
											my_para.torque_ref,
											my_para.kp,
											my_para.kd);

								printf("RS00 CTRL: Pos=%.2f, Vel=%.2f, Tor=%.2f, Kp=%.1f, Kd=%.1f\r\n",
									my_para.position_ref, my_para.velocity_ref,
									my_para.torque_ref, my_para.kp, my_para.kd);
								}
								break;

							// 以下为其他可选指令，不影响默认运控模�?
							case 0x02:  // 速度模式（非默认，保留备用）
								my_Order_mode.motor2_sudu = data_float;
								my_Order_mode.motor2_dianliu_limit = data_float_ext;
								RS00_WriteParamFloat(0x7F, 0xFD, 0x700A, my_Order_mode.motor2_sudu);
								printf("RS00 Velocity Mode: speed=%.3f, cur_limit=%.2f\n",
									   my_Order_mode.motor2_sudu, my_Order_mode.motor2_dianliu_limit);
								break;

							case 0x03:  // 力矩指令（预留）
								break;

							case 0x04:  // 单独设置Kp
								RS00_WriteParamFloat(0x7F, 0xFD, 0x701E, data_float);
								printf("RS00 Set LOC_KP = %.4f\r\n", data_float);
								break;

							case 0x05:  // 单独设置Kd/Ki
								RS00_WriteParamFloat(0x7F, 0xFD, 0x7020, data_float);
								printf("RS00 Set SPD_KI = %.4f\r\n", data_float);
								break;

							case 0x06:  // 电流限制
								printf("dianliu = %.4f\r\n", data_float);
								break;

							default:
								break;
						}
					}
				}
				else
				{
#if USART2_PROTOCOL_DEBUG
					printf("[USART2] checksum fail calc=%02X rx=%02X\r\n", checksum, g_rx_buffer[13]);
#endif
					USART2_PrintFrame("[USART2] bad frame:", g_rx_buffer, PROTOCOL_LENGTH);
				}
				
				// 重置状态机
				g_rx_state = RX_STATE_WAIT_HEAD1;
				break;
		}
		
		// 重新启动接收中断
		HAL_UART_Receive_IT(&huart2, (uint8_t *)g_usart2_rx_buffer, RXBUFFERSIZE);
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart->Instance == USART3)
	{
		g_usart3_dma_rx_count += Size;
		proto_transport_rx_process(g_usart3_dma_rx_buf, Size);
		(void)HAL_UARTEx_ReceiveToIdle_DMA(&huart3, g_usart3_dma_rx_buf, UART3_DMA_RX_BUF_SIZE);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2)
	{
#if USART2_PROTOCOL_DEBUG
		uint32_t error_code = HAL_UART_GetError(huart);
		printf("[USART2] error=0x%08lX restart RX\r\n", error_code);
#endif
		g_rx_state = RX_STATE_WAIT_HEAD1;
		HAL_UART_Receive_IT(&huart2, (uint8_t *)g_usart2_rx_buffer, RXBUFFERSIZE);
	}
	else if(huart->Instance == USART3)
	{
		uint32_t error_code = HAL_UART_GetError(huart);
		printf("[UART3] error=0x%08lX restart DMA\r\n", (unsigned long)error_code);
		(void)HAL_UARTEx_ReceiveToIdle_DMA(&huart3, g_usart3_dma_rx_buf, UART3_DMA_RX_BUF_SIZE);
	}
}


void uart2_SendDatas(uint8_t *pData, uint16_t Size)
{
//	HAL_UART_Transmit(&huart2, pData, Size, 100);
	
	if(uart_dma_busy)
	{
		return;
	}

	uart_dma_busy = 1;
	if(HAL_UART_Transmit_DMA(&huart2, pData, Size) != HAL_OK)
	{
		uart_dma_busy = 0;
	}
}


void uart_send_10float(float *data)
{
    uint8_t i;
    uint8_t *p;
    uint8_t sum = 0;

    uart_tx_buf[0] = 'm';
    uart_tx_buf[1] = 'j';

    p = &uart_tx_buf[2];

    for(i = 0; i < 10; i++)
    {
        memcpy(p, &data[i], 4);
        p += 4;
    }

    for(i = 0; i < 42; i++)
    {
        sum += uart_tx_buf[i];
    }

    uart_tx_buf[42] = sum;

    HAL_UART_Transmit(&huart1, uart_tx_buf, 43, 100);
}

/* Simple test frame sender for USART2
   Builds a minimal valid PROTOCOL frame and sends via DMA (or blocking if DMA busy). */
void USART2_SendTestFrame(void)
{
    uint8_t frame[PROTOCOL_LENGTH];
    uint8_t sum = 0;
    int i;

    frame[0] = PROTOCOL_HEAD1;
    frame[1] = PROTOCOL_HEAD2;
    frame[2] = 0x01; /* addr */
    frame[3] = 0x01; /* cmd1 */
    frame[4] = 0x01; /* cmd2 */
    /* data bytes - zero */
    for(i = 5; i < (PROTOCOL_LENGTH - 1); i++) frame[i] = 0x00;
    /* checksum: sum of bytes from addr (index 2) to last data (index PROTOCOL_LENGTH-2) */
    for(i = 2; i < (PROTOCOL_LENGTH - 1); i++) sum += frame[i];
    frame[PROTOCOL_LENGTH - 1] = sum;

    /* If DMA busy, send blocking as fallback */
    if(uart_dma_busy)
    {
        HAL_UART_Transmit(&huart2, frame, PROTOCOL_LENGTH, 100);
    }
    else
    {
        uart2_SendDatas(frame, PROTOCOL_LENGTH);
    }
}
/* USER CODE END 1 */
