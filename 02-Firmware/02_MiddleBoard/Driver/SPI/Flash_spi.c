/******************************************************************************
    文件名:   Flash_spi.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h"

SPI_HandleTypeDef g_spi1_handler; /* SPI2句柄 */
//-----------------------------------------------------------
//Description    : 设置SPI速度
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Flash_SPI_SetSpeed(uint8_t SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed)); 								/* 判断有效性 */
    __HAL_SPI_DISABLE(&g_spi1_handler);            	 								/* 关闭SPI */
    g_spi1_handler.Instance->CR1 &= 0XFFC7;        		 							/* 位3-5清零，用来设置波特率 */
    g_spi1_handler.Instance->CR1 |= SPI_BaudRatePrescaler << 3;     /* 设置SPI速度 */
    __HAL_SPI_ENABLE(&g_spi1_handler);             								  /* 使能SPI */
} 
//-----------------------------------------------------------
//Description    : Flash_SPI读取函数
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
uint8_t Flash_SPI_ReadWriteByte(uint8_t TxData)
{		
    uint8_t rxdata;
    HAL_SPI_TransmitReceive(&g_spi1_handler, &TxData, &rxdata, 1, 1000);
    return rxdata; /* 返回收到的数据 */				    
}

//-----------------------------------------------------------
//Description    : Flash_SPI配置初始化
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Flash_SPI_Init(void)
{
	GPIO_InitTypeDef gpio_init_struct;
	__HAL_RCC_SPI1_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();


	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_3);


	// PB4 - SPI1_MISO
	gpio_init_struct.Pin = GPIO_PIN_4;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP; // 推挽输出复用功能
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = GPIO_AF5_SPI2; // 根据你的STM32型号，复用功能的编号可能不同，例如STM32F4通常使用AF5
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);
 
    // 配置PB5为SPI1的MISO（复用功能）
    gpio_init_struct.Pin = GPIO_PIN_5;
    gpio_init_struct.Alternate = GPIO_AF5_SPI1; // 同样，确保这是正确的复用功能编号
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);

    // PB3 - SPI1_SCK
    gpio_init_struct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);

	__HAL_RCC_SPI1_CLK_ENABLE(); 										/* SPI2时钟使能 */

	g_spi1_handler.Instance = SPI1;                                		/* SPI2 */
	g_spi1_handler.Init.Mode = SPI_MODE_MASTER;                        	/* 设置SPI工作模式，设置为主模式 */
	g_spi1_handler.Init.Direction = SPI_DIRECTION_2LINES;              	/* 设置SPI单向或者双向的数据模式:SPI设置为双线模式 */
	g_spi1_handler.Init.DataSize = SPI_DATASIZE_8BIT;                  	/* 设置SPI的数据大小:SPI发送接收8位帧结构 */
	g_spi1_handler.Init.CLKPolarity = SPI_POLARITY_HIGH;               	/* 串行同步时钟的空闲状态为高电平 */
	g_spi1_handler.Init.CLKPhase = SPI_PHASE_2EDGE;                    	/* 串行同步时钟的第二个跳变沿（上升或下降）数据被采样 */
	g_spi1_handler.Init.NSS = SPI_NSS_SOFT;                            	/* NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制 */
	g_spi1_handler.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; 	/* 定义波特率预分频的值:波特率预分频值为256 */
	g_spi1_handler.Init.FirstBit = SPI_FIRSTBIT_MSB;                   	/* 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始 */
	g_spi1_handler.Init.TIMode = SPI_TIMODE_DISABLE;                   	/* 关闭TI模式 */
	g_spi1_handler.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;   	/* 关闭硬件CRC校验 */
	g_spi1_handler.Init.CRCPolynomial = 10;                             	/* CRC值计算的多项式 */
	HAL_SPI_Init(&g_spi1_handler);                                     	/* 初始化 */

	__HAL_SPI_ENABLE(&g_spi1_handler); /* 使能SPI2 */

	Flash_SPI_ReadWriteByte(0Xff); /* 启动传输, 实际上就是产生8个时钟脉冲, 达到清空DR的作用, 非必需 */
}
