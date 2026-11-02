/******************************************************************************
    文件名:   uart1.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h"
uint8_t u8Uart1TxBuffer[DF_UART1_REC_BUF_LEN];      //串口2发送缓冲区
uint8_t u8Uart1RxBuffer[DF_UART1_REC_BUF_LEN];      //串口2接收缓冲区
void Uart1DmaRxInit(void);
void Uart1DmaTxInit(void);
#define USART1_DATA_ADDRESS    ((uint32_t)&USART_DATA(USART1))
UART_HandleTypeDef huart1;                  		/* UART句柄 */
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
static uint8_t u8Uart1FrameBuffer[DF_UART1_REC_BUF_LEN];
static uint8_t u8Uart1SendBuffer[DF_UART1_REC_BUF_LEN];
static uint16_t u16Uart1FrameLen = 0;
static volatile uint8_t u8Uart1FrameReady = 0;
#define DF_QT_FRAME_LEN             14U
#define DF_QT_HEAD1                 0x6D
#define DF_QT_HEAD2                 0x6A
#define DF_QT_ADDR                  0x01
#define DF_FORCE_SENSOR_CMD_GROUP   0x21
#define DF_FORCE_SENSOR_ZERO_CMD    0x01
//-----------------------------------------------------------
//Description    : Uart1初始化配置
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Uart1Init(uint32_t u32baud)
{
	huart1.Instance = USART1;
	HAL_UART_DeInit(&huart1);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_USART1_CLK_DISABLE();
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

	HAL_NVIC_DisableIRQ(USART1_IRQn);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

	/* USART1 DMA DeInit */
	HAL_DMA_DeInit(&hdma_usart1_tx);
	HAL_DMA_DeInit(&hdma_usart1_rx);

	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	huart1.Instance = USART1;
	huart1.Init.BaudRate = u32baud;
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
	__HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE); //使能IDLE中断
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); //使能IDLE中断

	Uart1DmaRxInit();              				  //串口1DMA接收初始化
	Uart1DmaTxInit();

	HAL_UART_Receive_DMA(&huart1,u8Uart1RxBuffer,DF_UART1_REC_BUF_LEN);

	HAL_NVIC_EnableIRQ(USART1_IRQn);                      /* 使能USART1中断通道 */
	HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);              /* ESP32 command RX priority */
	/* DMA interrupt init */
	/* DMA1_Channel1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

//-----------------------------------------------------------
//Description    : Uart1 DMA接收初始化配置
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Uart1DmaRxInit(void)
{
	__HAL_RCC_DMAMUX1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	hdma_usart1_rx.Instance = DMA1_Channel2;
    hdma_usart1_rx.Init.Request = DMA_REQUEST_USART1_RX;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&huart1,hdmarx,hdma_usart1_rx);
}
//-----------------------------------------------------------
//Description    : Uart1 DMA发送初始化配置
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
 void Uart1DmaTxInit(void)
{
	__HAL_RCC_DMAMUX1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	 /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA1_Channel1;
    hdma_usart1_tx.Init.Request = DMA_REQUEST_USART1_TX;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }
    __HAL_LINKDMA(&huart1,hdmatx,hdma_usart1_tx);
}
//-----------------------------------------------------------
//Description    : Uart1 DMA发送函数
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Uart1DMASendFunction(uint8_t *u8Buf, uint16_t u16len)
{
	Uart1_RS485_TX_ENABLE();
	memcpy(u8Uart1TxBuffer, u8Buf, u16len);      
	if(HAL_UART_Transmit_DMA(&huart1,(uint8_t*)u8Uart1TxBuffer,u16len)!= HAL_OK) 
	{

	}
}
/**
  * @brief This function handles DMA1 channel6 global interrupt.
  */
/**
  * @brief This function handles DMA1 channel1 global interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

/**
  * @brief This function handles DMA1 channel2 global interrupt.
  */
void DMA1_Channel2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
}
//-----------------------------------------------------------
//Description    : Uart1 接收中断服务程序
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
static uint8_t Uart1QtFrameChecksum(const uint8_t *u8Buf)
{
    uint8_t u8Index;
    uint8_t u8Checksum = 0U;

    for (u8Index = 2U; u8Index < 13U; u8Index++)
    {
        u8Checksum = (uint8_t)(u8Checksum + u8Buf[u8Index]);
    }

    return u8Checksum;
}

static uint8_t Uart1IsValidQtFrame(const uint8_t *u8Buf, uint16_t u16Len)
{
    if ((u8Buf == NULL) || (u16Len != DF_QT_FRAME_LEN))
    {
        return 0U;
    }

    if ((u8Buf[0] != DF_QT_HEAD1) ||
        (u8Buf[1] != DF_QT_HEAD2) ||
        (u8Buf[2] != DF_QT_ADDR))
    {
        return 0U;
    }

    return (Uart1QtFrameChecksum(u8Buf) == u8Buf[13]) ? 1U : 0U;
}

static uint8_t Uart1IsForceSensorZeroCommand(const uint8_t *u8Buf, uint16_t u16Len)
{
    if (Uart1IsValidQtFrame(u8Buf, u16Len) == 0U)
    {
        return 0U;
    }

    if ((u8Buf[3] != DF_FORCE_SENSOR_CMD_GROUP) ||
        (u8Buf[4] != DF_FORCE_SENSOR_ZERO_CMD))
    {
        return 0U;
    }

    return ((u8Buf[5] | u8Buf[6] | u8Buf[7] | u8Buf[8]) != 0U) ? 1U : 0U;
}

void Uart1RxToUart4Task(void)
{
	uint16_t u16SendLen;
	uint32_t u32Primask;

	u32Primask = __get_PRIMASK();
	__disable_irq();

	if (u8Uart1FrameReady == 0U)
	{
		if (u32Primask == 0U)
		{
			__enable_irq();
		}
		return;
	}

	u16SendLen = u16Uart1FrameLen;
	memcpy(u8Uart1SendBuffer, u8Uart1FrameBuffer, u16SendLen);
	u8Uart1FrameReady = 0U;
	u16Uart1FrameLen = 0U;

	if (u32Primask == 0U)
	{
		__enable_irq();
	}

    if (Uart1IsForceSensorZeroCommand(u8Uart1SendBuffer, u16SendLen) != 0U)
    {
        (void)Uart3DR304RequestZeroAllAndSave();
        return;
    }

	Uart4DebugSendRxFrame(u8Uart1SendBuffer, u16SendLen);
}
void USART1_IRQHandler(void)
{
    uint16_t u16RceLen = 0;
	if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_IDLE)!= RESET)//如果接受到了一帧数据
	{ 
		__HAL_UART_CLEAR_IDLEFLAG(&huart1);//清除标志位
		HAL_UART_DMAStop(&huart1); //
		u16RceLen  =  __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);// 获取DMA中未传输的数据个数   
		u16RceLen =  DF_UART1_REC_BUF_LEN - u16RceLen; //总计数减去未传输的数据个数，得到已经接收的数据个数
		if ((u16RceLen > 0U) && (u16RceLen <= DF_UART1_REC_BUF_LEN))
		{
			memcpy(u8Uart1FrameBuffer, u8Uart1RxBuffer, u16RceLen);
			u16Uart1FrameLen = u16RceLen;
			u8Uart1FrameReady = 1U;
		}	
		HAL_UART_Receive_DMA(&huart1,u8Uart1RxBuffer,DF_UART1_REC_BUF_LEN);		
	}
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) != RESET)//
    {
		 Uart1_RS485_RX_ENABLE();
		
    }
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE) != RESET)//
    {

    }
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET)//
    {
		__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_ORE);
		
    }  
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE) != RESET)//
    {
		__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_NE);
    }
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE) != RESET)//
    {
		__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_FE);
	}	
    HAL_UART_IRQHandler(&huart1);	
}

