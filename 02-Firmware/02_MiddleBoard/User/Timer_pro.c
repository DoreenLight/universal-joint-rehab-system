/******************************************************************************
    文件名:   Timer_pro.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h"

//-----------------------------------------------------------
//Description    : 定时1ms处理任务
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Timer1msProFunction(void)
{

}
//-----------------------------------------------------------
//Description    : 定时5ms处理任务
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Timer5msProFunction(void)
{
}
//-----------------------------------------------------------
//Description    : 定时10ms处理任务
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Timer10msProFunction(void)
{
                     //RS485升级流程处理
}
//-----------------------------------------------------------
//Description    : 定时25ms处理任务
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Timer25msProFunction(void) 
{
}   
//-----------------------------------------------------------
//Description    : 定时50ms处理任务
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Timer50msProFunction(void) 
{
	static uint8_t u8DelayCnt = 0;
	static uint16_t u16DelayCnt1 = 0;
	if(++ u8DelayCnt > 19)
	{
		u8DelayCnt = 0;
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_6); 	
		
	}
	   	
    
}
//-----------------------------------------------------------
//Description    : 定时100ms处理任务
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void Timer100msProFunction(void)
{
    									
}


