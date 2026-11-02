#include "sys.h"  
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//系统时钟初始化	
//包括时钟设置/中断管理/GPIO设置等
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/2
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//********************************************************************************
//修改说明
//无
//////////////////////////////////////////////////////////////////////////////////  
volatile unsigned char hsefail;

//THUMB指令不支持汇编内联
//采用如下方法实现执行汇编指令WFI  
__asm void WFI_SET(void)
{
	WFI;		  
}
//关闭所有中断(但是不包括fault和NMI中断)
__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR	  
}
//开启所有中断
__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR  
}
//设置栈顶地址
//addr:栈顶地址
__asm void MSR_MSP(u32 addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}



//----------------------------------------------------------	 
u8 Sys_Clock_Set(u32 plln,u32 pllm,u32 pllp,u32 pllq)
{ 
		u16 retry=0;
		u8 status=0;
		RCC_CR |= 1<<16;				//HSE 开启 
		while(((RCC_CR&(1<<17))==0)&&(retry<0X1FFF)) retry++;//等待HSE RDY
		if(retry==0X1FFF) status=1;	    //HSE无法就绪
		else   
		{
			RCC_APB1ENR |= 1<<28;	   	//电源接口时钟使能
			PWR_CR |= 3<<14; 		   		//高性能模式,时钟可到168Mhz
			RCC_CFGR |= (0<<4)|(5<<10)|(4<<13);//HCLK 不分频;APB1 4分频;APB2 2分频. 
			RCC_CR &= ~(1<<24);	  //关闭主PLL
			RCC_PLLCFGR = pllm|(plln<<6)|(((pllp>>1)-1)<<16)|(pllq<<24)|(1<<22);			//配置主PLL,PLL时钟源来自HSE
			RCC_CR |= 1<<24;			//打开主PLL
			while((RCC_CR & (1<<25))==0);//等待PLL准备好 
			FLASH_ACR |= 1<<8;		    //指令预取使能.
			FLASH_ACR |= 1<<9;		    //指令cache使能.
			FLASH_ACR |= 1<<10;		//数据cache使能.
			FLASH_ACR |= 5<<0;		    //5个CPU等待周期. 
			RCC_CFGR &= ~(3<<0);		//清零
			RCC_CFGR |= 2<<0;		    //选择主PLL作为系统时钟	 
			while((RCC_CFGR&(3<<2))!=(2<<2));//等待主PLL作为系统时钟成功. 
		} 
		return status;
} 
//----------------------------------------------------------
void Stm32_Clock_Init(u32 plln,u32 pllm,u32 pllp,u32 pllq)
{  
		RCC_CR|=0x00000001;		//设置HSISON,开启内部高速RC振荡
		RCC_CFGR=0x00000000;		//CFGR清零 
		RCC_CR&=0xFEF6FFFF;		//HSE ON--关闭,CSS ON关闭,PLLON清零关闭 
		RCC_PLLCFGR=0x24003010;	//PLLCFGR恢复复位值 
		RCC_CR&=~(1<<18);			//HSEBYP清零,外部晶振不旁路
		RCC_CIR=0x00000000;		//禁止RCC时钟中断 
		Sys_Clock_Set(plln,pllm,pllp,pllq);//设置时钟 
}

void system_user()   //HSI ????
{
            RCC_DeInit();        //?????????  HSI  16M
            RCC_HSICmd(ENABLE);  //??HSI  16MHZ
            while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) ==RESET) ;  //?? HSI??

            // ????????????1,??????????
            // ?????????????
            RCC->APB1ENR |= RCC_APB1ENR_PWREN;
            PWR->CR |= PWR_CR_VOS;
    
            // HCLK = SYSCLK / 1
            RCC_HCLKConfig(RCC_SYSCLK_Div1);    //AHB = ????
            // PCLK2 = HCLK / 2
            RCC_PCLK2Config(RCC_HCLK_Div2);     //??,APB2 = AHB/2
            // PCLK1 = HCLK / 4
            RCC_PCLK1Config(RCC_HCLK_Div4);        //??,APB1 = AHB/4
    
        RCC_PLLCmd(DISABLE);
        //              ???          ????,????,pll
//        RCC_PLLConfig(RCC_PLLSource_HSI, 25, 336, 2, 7);
	RCC_PLLConfig(RCC_PLLSource_HSI, 16, 336, 2, 7);

            // ??PLL
            RCC_PLLCmd(ENABLE);
//      while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);   //??pll??
//            
//            RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);     //??PLL???????
//            while (RCC_GetSYSCLKSource() != 0x08);   // 
}

void Myrccconfig(void)
{
	//???:??RCC???
	RCC_DeInit();
	
	//???:??HSE????STM32?????
	RCC_HSEConfig(RCC_HSE_ON);
 
	//???:??HSE??,???????,??????
    while(RCC_GetFlagStatus(RCC_FLAG_HSERDY) != SET);
	
	//???:??flash,??????????.??:RCC????????,??????
	FLASH_PrefetchBufferCmd(ENABLE);//??FLASH??????
	FLASH_InstructionCacheCmd(ENABLE); //??????
	FLASH_DataCacheCmd(ENABLE);  //??????
	FLASH_SetLatency(FLASH_Latency_5);	//??5???
    
	//???:????HCLK???????(1)?????PCLK1?PCLK2?????(4?2)
	RCC_HCLKConfig(RCC_SYSCLK_Div2); // AHB
    RCC_PCLK1Config(RCC_HCLK_Div4);
    RCC_PCLK2Config(RCC_HCLK_Div2);
		
	//???:??PLL???????HSE,??PLLM?PLLN?PLLP?PLLQ?????
	RCC_PLLConfig(RCC_PLLSource_HSE,8,336,2,7);
    RCC_PLLCmd(ENABLE);//pll??;
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) != SET);
    
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while(RCC_GetSYSCLKSource() != 0x08);
}

void HSI_SetSysClock(uint32_t m, uint32_t n, uint32_t p, uint32_t q)	
{
  __IO uint32_t HSIStartUpStatus = 0;
	
  RCC_DeInit();
  
  //HSI, HSI=16M
	RCC_HSICmd(ENABLE);
	
	HSIStartUpStatus = RCC->CR & RCC_CR_HSIRDY;
	
  if (HSIStartUpStatus == RCC_CR_HSIRDY)
  {
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;
		
		// HCLK = SYSCLK / 1
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		
		// PCLK2 = HCLK / 2
		RCC_PCLK2Config(RCC_HCLK_Div2);
		
		// PCLK1 = HCLK / 4
		RCC_PCLK1Config(RCC_HCLK_Div4);
		
		RCC_PLLConfig(RCC_PLLSource_HSI, m, n, p, q);
		
		RCC_PLLCmd(ENABLE);
  
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }   
 
    FLASH->ACR = FLASH_ACR_PRFTEN 
		            | FLASH_ACR_ICEN 
		            |FLASH_ACR_DCEN 
		            |FLASH_ACR_LATENCY_5WS;
		
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
 
    while (RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }
  else
  { 
    while (1)
    {
    }
  }
}











