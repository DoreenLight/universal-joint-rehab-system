/******************************************************************************
    文件名:   main.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h"
uint8_t u8Gu8TaskDelayTimeTab[DF_Task_NUM]={DF_TASK1_TIMER,DF_TASK2_TIMER,DF_TASK3_TIMER,DF_TASK4_TIMER,DF_TASK5_TIMER,DF_TASK6_TIMER};	 //分时处理定时  分成5个任务 5ms 10ms 25ms 50ms 100ms执行一次的时间
uint8_t u8Gu8TaskDelayRunTab[DF_Task_NUM]={0x01,0x01,0x01,0x01,0x01}; 
uint8_t u8_sys_addr_value=0;
uint8_t u8RunStep = 0;	
const uint32_t u32TestTab[110];
uint32_t u32TestVl = 0;


int main(void)
{
	HAL_Init();                                 			/* 初始化HAL库 */
	sys_stm32_clock_init(85, 2, 2, 4, 8);                   /* 设置时钟,170Mhz */
	delay_init(170);                            			/* 延时初始化 */
	delay_ms(100);
	Exterl_Dev_GPIO_Config();
	Uart4Init(DF_UART4_BAUD_RARE);
	Uart1Init(DF_UART1_BAUD_RARE);
	Uart3Init(DF_UART3_BAUD_RARE);

	//需要校准力传感器就解除注释
	//	delay_ms(500);
	//	Uart3DR304ZeroAll();
	//	delay_ms(100);
	//	Uart3DR304SaveParameters();
	//	delay_ms(100);
	TIM3_Init(10 - 1, 17000 - 1);    						/* 10Khz的计数频率，计数5K次为500ms */	
	MX25L1606E_Flash_Init();    							//初始化MX25L1606E
	while (1)
	{       
		Uart1RxToUart4Task();
		Uart3RxToUart4Task();
		Uart3PollTask();
		/***********************************************************************************
		***********************************************************************************/
		if(u8Gu8TaskDelayRunTab[0])  	 							    //1ms
		{
			Timer1msProFunction();	 
			u8Gu8TaskDelayRunTab[0] = 0x00;
			u8Gu8TaskDelayTimeTab[0] = DF_TASK1_TIMER;                  //运行完一次，重新计时  1*1ms=5ms
		}
		if(u8Gu8TaskDelayRunTab[1])   							        //5ms
		{
			Timer5msProFunction();
			u8Gu8TaskDelayRunTab[1] = 0x00;
			u8Gu8TaskDelayTimeTab[1] = DF_TASK2_TIMER;                  //运行完一次，重新计时 5*1ms=5ms
		}					
		if(u8Gu8TaskDelayRunTab[2])   							        //10ms
		{
			Timer10msProFunction();
			u8Gu8TaskDelayRunTab[2] = 0x00;
			u8Gu8TaskDelayTimeTab[2] = DF_TASK3_TIMER;                  //运行完一次，重新计时   10*1ms=10ms		 
		}
		if(u8Gu8TaskDelayRunTab[3])   							        //25ms
		{			   
			Timer25msProFunction();
			u8Gu8TaskDelayRunTab[3] = 0x00;
			u8Gu8TaskDelayTimeTab[3] = DF_TASK4_TIMER;                  //运行完一次，重新计时     25*1ms=25ms		 
		}
		if(u8Gu8TaskDelayRunTab[4])   							        //50ms
		{   
			Timer50msProFunction();
			u8Gu8TaskDelayRunTab[4] = 0x00;
			u8Gu8TaskDelayTimeTab[4] = DF_TASK5_TIMER;                  //运行完一次，重新计时      50*1ms=50ms 
		}
		if(u8Gu8TaskDelayRunTab[5])   							        //100ms
		{   
			Timer100msProFunction();			
			u8Gu8TaskDelayRunTab[5] = 0x00;
			u8Gu8TaskDelayTimeTab[5] = DF_TASK6_TIMER;                  //运行完一次，重新计时       100*1ms=100ms 	   
		}	
	}
}

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}


