/******************************************************************************
    File: uart3.c
    Version: v1.0
******************************************************************************/
#include "main.h"

uint8_t u8Uart3TxBuffer[DF_UART3_REC_BUF_LEN];
uint8_t u8Uart3RxBuffer[DF_UART3_REC_BUF_LEN];

UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

static volatile uint8_t u8Uart3WaitingResponse = 0;
static uint32_t u32Uart3LastPollTick = 0;
static uint32_t u32Uart3RequestTick = 0;
#define UART3_QUEUE_SIZE 8U
volatile Uart3Diagnostics g_uart3_diag;
static uint8_t rx_stream[DF_DR304_RESPONSE_MAX_LEN];
static uint16_t rx_used, dma_position;
static uint8_t frame_queue[UART3_QUEUE_SIZE][DF_DR304_RESPONSE_MAX_LEN];
static volatile uint8_t queue_head, queue_tail;
static volatile uint8_t rx_recovery_pending;
static uint32_t recovery_tick;
static void Uart3ConsumeDma(void);
static void Uart3Feed(const uint8_t *data, uint16_t length);
static void Uart3RecoverRx(void);
static volatile uint8_t u8Uart3InitOk = 0;
static volatile uint8_t u8Uart3TxBusy = 0;
static volatile uint8_t u8Uart3ZeroSaveStep = 0;

HAL_StatusTypeDef Uart3DmaRxInit(void);
HAL_StatusTypeDef Uart3DmaTxInit(void);
static void Uart3AppendModbusCrc(uint8_t *u8Buf, uint16_t u16Len);
static uint8_t Uart3IsModbusResponse(const uint8_t *u8Buf, uint16_t u16Len);

void Uart3Init(uint32_t u32baud)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    huart3.Instance = USART3;
    u8Uart3InitOk = 0U;

    HAL_UART_DeInit(&huart3);
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_11);
    HAL_NVIC_DisableIRQ(USART3_IRQn);

    HAL_DMA_DeInit(&hdma_usart3_tx);
    HAL_DMA_DeInit(&hdma_usart3_rx);

    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart3.Instance = USART3;
    huart3.Init.BaudRate = u32baud;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        return;
    }

    __HAL_UART_DISABLE_IT(&huart3, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

    if (Uart3DmaRxInit() != HAL_OK)
    {
        return;
    }

    if (Uart3DmaTxInit() != HAL_OK)
    {
        return;
    }

    rx_used = dma_position = 0U;
    queue_head = queue_tail = 0U;
    rx_recovery_pending = 0U;
    if (HAL_UART_Receive_DMA(&huart3, u8Uart3RxBuffer, DF_UART3_REC_BUF_LEN) != HAL_OK)
    {
        g_uart3_diag.rx_restart_errors++;
        rx_recovery_pending = 1U;
    }

    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_NVIC_SetPriority(USART3_IRQn, 4, 0);

    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 4, 0); /* serialize RX consumers */
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

    Uart3_RS485_RX_ENABLE();
    u8Uart3WaitingResponse = 0;
    u32Uart3LastPollTick = HAL_GetTick();
    u32Uart3RequestTick = 0;
    recovery_tick = HAL_GetTick();
    u8Uart3TxBusy = 0U;
    u8Uart3ZeroSaveStep = 0U;
    u8Uart3InitOk = 1U;
}

void Uart3PollTask(void)
{
    uint32_t u32NowTick;

    if (u8Uart3InitOk == 0U)
    {
        return;
    }

    Uart3RecoverRx();
    if (rx_recovery_pending != 0U) return;
    u32NowTick = HAL_GetTick();
    if (u8Uart3TxBusy != 0U)
    {
        if ((uint32_t)(u32NowTick - u32Uart3RequestTick) >= DF_DR304_RESPONSE_TIMEOUT_MS)
        {
            g_uart3_diag.tx_timeouts++;
            (void)HAL_UART_AbortTransmit(&huart3);
            u8Uart3TxBusy = 0U;
            u8Uart3WaitingResponse = 0U;
            Uart3_RS485_RX_ENABLE();
            u32Uart3LastPollTick = u32NowTick;
        }
        return;
    }

    if (u8Uart3WaitingResponse != 0U)
    {
        if ((uint32_t)(u32NowTick - u32Uart3RequestTick) >= DF_DR304_RESPONSE_TIMEOUT_MS)
        {
            g_uart3_diag.response_timeouts++;
            u8Uart3WaitingResponse = 0U;
            u32Uart3LastPollTick = u32NowTick;
        }
        return;
    }

    if (u8Uart3ZeroSaveStep != 0U)
    {
        if (u8Uart3ZeroSaveStep == 1U)
        {
            if (Uart3DR304ZeroAll() == HAL_OK)
            {
                u8Uart3ZeroSaveStep = 2U;
            }
            else
            {
                u8Uart3ZeroSaveStep = 0U;
                u32Uart3LastPollTick = u32NowTick;
            }
            return;
        }

        if (u8Uart3ZeroSaveStep == 2U)
        {
            if (Uart3DR304SaveParameters() == HAL_OK)
            {
                u8Uart3ZeroSaveStep = 3U;
            }
            else
            {
                u8Uart3ZeroSaveStep = 0U;
                u32Uart3LastPollTick = u32NowTick;
            }
            return;
        }

        u8Uart3ZeroSaveStep = 0U;
        u32Uart3LastPollTick = u32NowTick;
        return;
    }

    if ((uint32_t)(u32NowTick - u32Uart3LastPollTick) < DF_DR304_POLL_INTERVAL_MS)
    {
        return;
    }

    if (Uart3ModbusReadHoldingRegisters(DF_DR304_MODBUS_ADDR, DF_DR304_VALUE_REG_ADDR, DF_DR304_VALUE_REG_NUM) == HAL_OK)
    {
        u8Uart3WaitingResponse = 1U;
        u32Uart3RequestTick = u32NowTick;
    }
    else
    {
        u32Uart3LastPollTick = u32NowTick;
    }
}

void Uart3RxToUart4Task(void)
{
    uint8_t frame[DF_DR304_RESPONSE_MAX_LEN];
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (queue_head == queue_tail)
    {
        if (mask == 0U) __enable_irq();
        return;
    }
    memcpy(frame, frame_queue[queue_tail], sizeof(frame));
    queue_tail = (uint8_t)((queue_tail + 1U) % UART3_QUEUE_SIZE);
    if (mask == 0U) __enable_irq();
#if (DF_DR304_FORWARD_TO_UART4 != 0)
    if (Uart4SendFrame(frame, sizeof(frame)) == HAL_OK)
        g_uart3_diag.forwarded_frames++;
    else
        g_uart3_diag.forward_errors++;
#endif
}

HAL_StatusTypeDef Uart3DmaRxInit(void)
{
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_usart3_rx.Instance = DMA1_Channel5;
    hdma_usart3_rx.Init.Request = DMA_REQUEST_USART3_RX;
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart3_rx.Init.Priority = DMA_PRIORITY_MEDIUM;

    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK)
    {
        return HAL_ERROR;
    }

    __HAL_LINKDMA(&huart3, hdmarx, hdma_usart3_rx);
    return HAL_OK;
}

HAL_StatusTypeDef Uart3DmaTxInit(void)
{
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_usart3_tx.Instance = DMA1_Channel6;
    hdma_usart3_tx.Init.Request = DMA_REQUEST_USART3_TX;
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;
    hdma_usart3_tx.Init.Priority = DMA_PRIORITY_MEDIUM;

    if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK)
    {
        return HAL_ERROR;
    }

    __HAL_LINKDMA(&huart3, hdmatx, hdma_usart3_tx);
    return HAL_OK;
}

HAL_StatusTypeDef Uart3RS485SendFrame(const uint8_t *u8Buf, uint16_t u16len)
{
    HAL_StatusTypeDef eStatus;

    if ((u8Uart3InitOk == 0U) || (u8Buf == NULL) || (u16len == 0) || (u16len > DF_UART3_REC_BUF_LEN))
    {
        return HAL_ERROR;
    }

    if (u8Uart3TxBusy != 0U || rx_recovery_pending != 0U) return HAL_BUSY;
    u32Uart3RequestTick = HAL_GetTick();
    Uart3_RS485_TX_ENABLE();
    memcpy(u8Uart3TxBuffer, u8Buf, u16len);
    u8Uart3TxBusy = 1U;

    eStatus = HAL_UART_Transmit_DMA(&huart3, (uint8_t *)u8Uart3TxBuffer, u16len);
    if (eStatus != HAL_OK)
    {
        u8Uart3TxBusy = 0U;
        Uart3_RS485_RX_ENABLE();
    }

    return eStatus;
}

void Uart3DMASendFunction(uint8_t *u8Buf, uint16_t u16len)
{
    (void)Uart3RS485SendFrame(u8Buf, u16len);
}

HAL_StatusTypeDef Uart3ModbusReadHoldingRegisters(uint8_t u8Addr, uint16_t u16RegAddr, uint16_t u16RegNum)
{
    uint8_t u8Frame[8];

    if ((u8Addr == 0U) || (u16RegNum == 0U) || (u16RegNum > 125U))
    {
        return HAL_ERROR;
    }

    u8Frame[0] = u8Addr;
    u8Frame[1] = 0x03;
    u8Frame[2] = (uint8_t)(u16RegAddr >> 8);
    u8Frame[3] = (uint8_t)u16RegAddr;
    u8Frame[4] = (uint8_t)(u16RegNum >> 8);
    u8Frame[5] = (uint8_t)u16RegNum;
    Uart3AppendModbusCrc(u8Frame, 6);

    return Uart3RS485SendFrame(u8Frame, sizeof(u8Frame));
}

HAL_StatusTypeDef Uart3ModbusWriteSingleRegister(uint8_t u8Addr, uint16_t u16RegAddr, uint16_t u16Value)
{
    uint8_t u8Frame[8];

    if (u8Addr == 0U)
    {
        return HAL_ERROR;
    }

    u8Frame[0] = u8Addr;
    u8Frame[1] = 0x06;
    u8Frame[2] = (uint8_t)(u16RegAddr >> 8);
    u8Frame[3] = (uint8_t)u16RegAddr;
    u8Frame[4] = (uint8_t)(u16Value >> 8);
    u8Frame[5] = (uint8_t)u16Value;
    Uart3AppendModbusCrc(u8Frame, 6);

    return Uart3RS485SendFrame(u8Frame, sizeof(u8Frame));
}

HAL_StatusTypeDef Uart3ModbusWriteMultipleRegisters(uint8_t u8Addr, uint16_t u16RegAddr, const uint16_t *u16Values, uint16_t u16RegNum)
{
    uint8_t u8Frame[DF_UART3_REC_BUF_LEN];
    uint16_t u16ByteNum;
    uint16_t u16Index;
    uint16_t u16Offset;

    if ((u8Addr == 0U) || (u16Values == NULL) || (u16RegNum == 0U) || (u16RegNum > 123U))
    {
        return HAL_ERROR;
    }

    u16ByteNum = u16RegNum * 2U;
    if ((uint16_t)(9U + u16ByteNum) > DF_UART3_REC_BUF_LEN)
    {
        return HAL_ERROR;
    }

    u8Frame[0] = u8Addr;
    u8Frame[1] = 0x10;
    u8Frame[2] = (uint8_t)(u16RegAddr >> 8);
    u8Frame[3] = (uint8_t)u16RegAddr;
    u8Frame[4] = (uint8_t)(u16RegNum >> 8);
    u8Frame[5] = (uint8_t)u16RegNum;
    u8Frame[6] = (uint8_t)u16ByteNum;

    u16Offset = 7U;
    for (u16Index = 0U; u16Index < u16RegNum; u16Index++)
    {
        u8Frame[u16Offset++] = (uint8_t)(u16Values[u16Index] >> 8);
        u8Frame[u16Offset++] = (uint8_t)u16Values[u16Index];
    }

    Uart3AppendModbusCrc(u8Frame, u16Offset);
    u16Offset += 2U;

    return Uart3RS485SendFrame(u8Frame, u16Offset);
}

HAL_StatusTypeDef Uart3DR304SetSampleRate(uint8_t u8RateCode)
{
    HAL_StatusTypeDef eStatus;
    uint16_t u16Value[2];

    if (u8RateCode > 7U)
    {
        return HAL_ERROR;
    }

    if (u8Uart3WaitingResponse != 0U)
    {
        return HAL_BUSY;
    }

    u16Value[0] = 0U;
    u16Value[1] = u8RateCode;
    eStatus = Uart3ModbusWriteMultipleRegisters(DF_DR304_MODBUS_ADDR, DF_DR304_SAMPLE_RATE_REG_ADDR, u16Value, 2U);
    if (eStatus == HAL_OK)
    {
        u8Uart3WaitingResponse = 1U;
        u32Uart3RequestTick = HAL_GetTick();
    }
    return eStatus;
}

HAL_StatusTypeDef Uart3DR304RequestZeroAllAndSave(void)
{
    if (u8Uart3InitOk == 0U)
    {
        return HAL_ERROR;
    }

    if (u8Uart3ZeroSaveStep != 0U)
    {
        return HAL_BUSY;
    }

    u8Uart3ZeroSaveStep = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef Uart3DR304SaveParameters(void)
{
    HAL_StatusTypeDef eStatus;
    uint16_t u16Value[2];

    if (u8Uart3WaitingResponse != 0U)
    {
        return HAL_BUSY;
    }

    u16Value[0] = 0U;
    u16Value[1] = 40U;
    eStatus = Uart3ModbusWriteMultipleRegisters(DF_DR304_MODBUS_ADDR, DF_DR304_SAVE_CMD_REG_ADDR, u16Value, 2U);
    if (eStatus == HAL_OK)
    {
        u8Uart3WaitingResponse = 1U;
        u32Uart3RequestTick = HAL_GetTick();
    }
    return eStatus;
}

HAL_StatusTypeDef Uart3DR304ZeroAll(void)
{
    HAL_StatusTypeDef eStatus;
    uint16_t u16Value[2];

    if (u8Uart3WaitingResponse != 0U)
    {
        return HAL_BUSY;
    }

    u16Value[0] = 0U;
    u16Value[1] = 7U;   // 7 = all channels zero

    eStatus = Uart3ModbusWriteMultipleRegisters(DF_DR304_MODBUS_ADDR,
                                                DF_DR304_SAVE_CMD_REG_ADDR,
                                                u16Value,
                                                2U);

    if (eStatus == HAL_OK)
    {
        u8Uart3WaitingResponse = 1U;
        u32Uart3RequestTick = HAL_GetTick();
    }

    return eStatus;
}

static void Uart3AppendModbusCrc(uint8_t *u8Buf, uint16_t u16Len)
{
    uint16_t u16Crc;

    u16Crc = Modbus_CalCRC(u8Buf, u16Len);
    u8Buf[u16Len] = (uint8_t)u16Crc;
    u8Buf[u16Len + 1U] = (uint8_t)(u16Crc >> 8);
}

static uint8_t Uart3IsModbusResponse(const uint8_t *u8Buf, uint16_t u16Len)
{
    uint16_t u16CrcCalc;
    uint16_t u16CrcRx;
    uint16_t u16ExpectedLen;

    if ((u8Buf == NULL) || (u16Len < 5U))
    {
        return 0U;
    }

    if (u8Buf[0] != DF_DR304_MODBUS_ADDR)
    {
        return 0U;
    }

    if (u8Buf[1] == 0x03U)
    {
        if (u8Buf[2] > DF_DR304_VALUE_BYTE_COUNT)
        {
            return 0U;
        }

        u16ExpectedLen = (uint16_t)u8Buf[2] + 5U;
        if ((u16Len != u16ExpectedLen) || (u16Len > DF_DR304_RESPONSE_MAX_LEN))
        {
            return 0U;
        }
    }
    else if (u8Buf[1] == (0x03U | 0x80U))
    {
        if (u16Len != 5U)
        {
            return 0U;
        }
    }
    else
    {
        return 0U;
    }

    u16CrcCalc = Modbus_CalCRC((uint8_t *)u8Buf, (uint16_t)(u16Len - 2U));
    u16CrcRx = (uint16_t)u8Buf[u16Len - 2U] | ((uint16_t)u8Buf[u16Len - 1U] << 8);

    return (u16CrcCalc == u16CrcRx) ? 1U : 0U;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        u8Uart3TxBusy = 0U;
        Uart3_RS485_RX_ENABLE();
    }
}

/* Called by equal-priority USART3 and DMA RX interrupts only. Circular DMA
 * remains armed across IDLE events; half/full IRQs prevent a full-lap ambiguity. */
static void Uart3ConsumeDma(void)
{
    uint16_t pos;
    if (rx_recovery_pending != 0U) return;
    pos = (uint16_t)(DF_UART3_REC_BUF_LEN - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx));
    if (pos >= DF_UART3_REC_BUF_LEN) pos = 0U;
    if (pos < dma_position)
    {
        Uart3Feed(&u8Uart3RxBuffer[dma_position], DF_UART3_REC_BUF_LEN - dma_position);
        dma_position = 0U;
    }
    if (pos > dma_position)
        Uart3Feed(&u8Uart3RxBuffer[dma_position], pos - dma_position);
    dma_position = pos;
}

static void Uart3Feed(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    for (i = 0U; i < length; ++i)
    {
        g_uart3_diag.rx_bytes++;
        rx_stream[rx_used++] = data[i];
        while (rx_used != 0U)
        {
            if (rx_stream[0] == DF_DR304_MODBUS_ADDR &&
                (rx_used < 2U || rx_stream[1] == 0x03U) &&
                (rx_used < 3U || rx_stream[2] == DF_DR304_VALUE_BYTE_COUNT))
            {
                uint8_t next;
                if (rx_used < DF_DR304_RESPONSE_MAX_LEN) break;
                if (Uart3IsModbusResponse(rx_stream, rx_used) != 0U)
                {
                    g_uart3_diag.valid_frames++;
                    g_uart3_diag.last_valid_tick = HAL_GetTick();
                    u8Uart3WaitingResponse = 0U;
                    u32Uart3LastPollTick = HAL_GetTick();
                    next = (uint8_t)((queue_head + 1U) % UART3_QUEUE_SIZE);
                    if (next == queue_tail) g_uart3_diag.queue_overflows++;
                    else
                    {
                        memcpy(frame_queue[queue_head], rx_stream, rx_used);
                        queue_head = next;
                    }
                    rx_used = 0U;
                    break;
                }
                g_uart3_diag.crc_errors++;
            }
            memmove(rx_stream, rx_stream + 1, --rx_used);
            g_uart3_diag.discarded_bytes++;
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        g_uart3_diag.uart_errors++;
        g_uart3_diag.last_uart_error = huart->ErrorCode;
        rx_recovery_pending = 1U;
    }
}

/* Main-loop retry, no changes to transmitter or zero/save commands. */
static void Uart3RecoverRx(void)
{
    uint32_t now = HAL_GetTick();
    if (rx_recovery_pending == 0U || (uint32_t)(now - recovery_tick) < 20U) return;
    recovery_tick = now;
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Channel5_IRQn);
    if (HAL_UART_AbortReceive(&huart3) == HAL_OK)
    {
        rx_used = dma_position = 0U;
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
        if (HAL_UART_Receive_DMA(&huart3, u8Uart3RxBuffer, DF_UART3_REC_BUF_LEN) == HAL_OK)
        {
            rx_recovery_pending = 0U;
            g_uart3_diag.rx_recoveries++;
        }
        else g_uart3_diag.rx_restart_errors++;
    }
    else g_uart3_diag.rx_restart_errors++;
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

void DMA1_Channel5_IRQHandler(void)
{
    /* HAL handles transfer errors before any bytes are accepted. */
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
    Uart3ConsumeDma();
}

void DMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
        Uart3ConsumeDma();
    }
}
