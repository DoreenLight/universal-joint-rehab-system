/******************************************************************************
    文件名:   extern_device_control.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h"

/***********************************************************************
***
*** 函数功能:外设控制引脚配置
***
***********************************************************************/
void Exterl_Dev_GPIO_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	__HAL_RCC_GPIOA_CLK_ENABLE(); 								/*开启LED相关的GPIO外设时钟*/
	__HAL_RCC_GPIOB_CLK_ENABLE(); 
	__HAL_RCC_GPIOC_CLK_ENABLE(); 
	__HAL_RCC_GPIOD_CLK_ENABLE(); 

	GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;				/*选择要控制的GPIO引脚*/		
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  				/*设置引脚的输出类型为推挽输出*/
	GPIO_InitStruct.Pull  = GPIO_PULLUP;						/*设置引脚为上拉模式*/
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;				/*设置引脚速率为高速 */  
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);						/*调用库函数，使用上面配置的GPIO_InitStructure初始化GPIO*/ 
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_6,GPIO_PIN_SET);     
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_SET);  
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_SET);
	
	
	/**********************RS485收发使用****************************************/
	GPIO_InitStruct.Pin = GPIO_PIN_9;				/*选择要控制的GPIO引脚*/		
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  				/*设置引脚的输出类型为推挽输出*/
	GPIO_InitStruct.Pull  = GPIO_PULLUP;						/*设置引脚为上拉模式*/
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;				/*设置引脚速率为高速 */  
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);						/*调用库函数，使用上面配置的GPIO_InitStructure初始化GPIO*/ 


    GPIO_InitStruct.Pin = GPIO_PIN_1;				/*选择要控制的GPIO引脚*/		
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  				/*设置引脚的输出类型为推挽输出*/
	GPIO_InitStruct.Pull  = GPIO_PULLUP;						/*设置引脚为上拉模式*/
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;				/*设置引脚速率为高速 */  
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);						/*调用库函数，使用上面配置的GPIO_InitStructure初始化GPIO*/ 

    GPIO_InitStruct.Pin = GPIO_PIN_2;				/*选择要控制的GPIO引脚*/		
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  				/*设置引脚的输出类型为推挽输出*/
	GPIO_InitStruct.Pull  = GPIO_PULLUP;						/*设置引脚为上拉模式*/
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;				/*设置引脚速率为高速 */  
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);						/*调用库函数，使用上面配置的GPIO_InitStructure初始化GPIO*/ 
	Uart1_RS485_RX_ENABLE();   
	Uart2_RS485_RX_ENABLE();  
}
/***********************************************************************
***
*** 函数功能:RS485通信接收使能
***
***********************************************************************/
void Uart1_RS485_RX_ENABLE(void)
{
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
}
/***********************************************************************
***
*** 函数功能:RS485通信发送使能
***
***********************************************************************/
void Uart1_RS485_TX_ENABLE(void)
{
     HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET);
}
/***********************************************************************
***
*** 函数功能:RS485通信接收使能
***
***********************************************************************/
void Uart2_RS485_RX_ENABLE(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET);
}
/***********************************************************************
***
*** 函数功能:RS485通信发送使能
***
***********************************************************************/
void Uart2_RS485_TX_ENABLE(void)
{
     HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);
}
/***********************************************************************
***
*** 函数功能:RS485通信接收使能
***
***********************************************************************/
void Uart3_RS485_RX_ENABLE(void)
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_2,GPIO_PIN_RESET);
}
/***********************************************************************
***
*** 函数功能:RS485通信发送使能
***
***********************************************************************/
void Uart3_RS485_TX_ENABLE(void)
{
     HAL_GPIO_WritePin(GPIOB,GPIO_PIN_2,GPIO_PIN_SET); 		//
}
