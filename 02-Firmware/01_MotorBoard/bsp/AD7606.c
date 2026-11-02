#include "delay.h"
#include "AD7606.h"
#include <stdio.h>
//#include "flash_param.h"
//#include "iwdg.h"

//void delay_ns(u32 nns)
//{
//	do{
//		nns --;
//	} while (nns);
//}




//// 使用DWT实现精确延迟（如果MCU支持）
//void delay_ns(uint32_t ns)
//{
//    // 根据系统时钟频率计算循环次数
//    uint32_t cycles = (SystemCoreClock / 1000000) * ns / 1000;
//    if(cycles < 1) cycles = 1;
//    
//    while(cycles--)
//    {
//        __NOP();
//    }
//}
//-----------------------------------------------------------------
// 初始化程序区
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// void GPIO_AD7606_Configuration(void)
//-----------------------------------------------------------------
//
// 函数功能: AD7606引脚配置函数
// 入口参数: 无
// 返回参数: 无
// 全局变量: 无
// 调用模块: RCC_APB2PeriphClockCmd(); GPIO_Init();
// 注意事项: 用GPIO驱动方式和FSMC驱动方式下的引脚配置不一样
//			
//-----------------------------------------------------------------
void GPIO_AD7606_Configuration(void)
{ 
//	GPIO_InitTypeDef  GPIO_InitStructure;

//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOE, ENABLE);//使能PD,PE,PF,PG时钟  
//	
//	//AD7606输入电压范围配置，低电平输入±5v；高电平输入正负10v
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	GPIO_ResetBits(GPIOA, GPIO_Pin_2);
//	//GPIO_SetBits(GPIOA, GPIO_Pin_2);

//	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	// AD7606 
//	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	// 控制线配置 
//	//OSI0
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
////	//OSI1
////	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
////	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
////	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
////	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
////	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
////	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
//	//OSI2
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
//	//SERI
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化
	
//	//STBY
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
//	//CONVA
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
//	//CONVB
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
//	//RESET
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
//	//RD
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化
//	
//	//CS1
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化
//	cs1_Set;
//	
//	//BUSY
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化
//	
//	//FRSTDATA
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输出模式
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
//	
//	//数据输入口
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14|GPIO_Pin_15|GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(GPIOE, &GPIO_InitStructure);//初始化
}

//-----------------------------------------------------------------
// void AD7606_Init(void)
//-----------------------------------------------------------------
//
// 函数功能: AD7606初始化函数
// 入口参数: 无
// 返回参数: 无
// 全局变量: 无
// 调用模块:    
// 注意事项: 无
//-----------------------------------------------------------------
void AD7606_Init(void)
{
	convstA_Set;
	convstB_Set;
	delay_ms(1);
//	STby_Set;
	
	rd_Set;
	cs1_Set;	
	
	OS10_Reset;
	OS11_Reset;
	OS12_Reset;
	range_5V;

	AD7606_reset();  
	delay_ms(1);
	AD7606_startconvst();	
}

/*   * 名称：AD7606_startconvst()  * 功能：启动转换  */  
void AD7606_startconvst(void)
{  
	convstA_Reset;	
	convstB_Reset;
	
	delay_ns(1000);
	convstA_Set;
	convstB_Set;
}
  
/*   * 名称：AD7606_reset()  * 功能：复位模块  */
void AD7606_reset(void) 
{ 
	rst_Reset;
	delay_ns(1000);
	rst_Set; 
	delay_ns(1000);
	rst_Reset; 
}  

/* 
* 名称：AD7606_read_data() 
* 功能：读取数据 
* 返回值：返回一个结构体指针，该指针为指向结构体数组的首地址  
*/ 
void AD7606_read_data(int16_t * DB_data) 
{  
	u8 i;
	
	s16 DATA = 0;//, temp = 0;	

//	unsigned int tick1 = 0, tick2 = 0;
	
//	tick1=SysTick->VAL;	
	
	cs1_Reset; 

	delay_ns(10);	
	
	for(i=0;i<8;i++)  
	{
		rd_Reset;
		delay_ns(2);
		rd_Set;
		delay_ns(2);
//		*(DB_data ++) = ADC_DATA_REG;
		
		//数据线配置
	 /** FMC GPIO Configuration  
  PE7   ------> FMC_DA4
  PE8   ------> FMC_DA5
  PE9   ------> FMC_DA6
  PE10   ------> FMC_DA7
  PE11   ------> FMC_DA8
  PE12   ------> FMC_DA9
  PE13   ------> FMC_DA10
  PE14   ------> FMC_DA11
  PE15   ------> FMC_DA12
  PD8   ------> FMC_DA13
  PD9   ------> FMC_DA14
  PD10   ------> FMC_DA15
  PD14   ------> FMC_DA0
  PD15   ------> FMC_DA1
  PD0   ------> FMC_DA2
  PD1   ------> FMC_DA3
  */
	
//		DATA = ((GPIO_ReadInputData(GPIOE) & 0xFF80) >> 3); 
//		temp = GPIO_ReadInputData(GPIOD);

//		DATA |= ((temp & 0x0700) << 5);  //PD8 9 10
//		DATA |= ((temp & 0xC000) >> 14); //PD14 15
//		DATA |= ((temp & 0x0003) << 2);  //PD0 1
//		*(DB_data ++) = DATA;

//		// 修改为（HAL）：
		uint16_t data_e, data_d;
		HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7); // 需要逐个读取或使用寄存器直接访问

		// 建议使用寄存器方式提高效率：
		data_e = GPIOE->IDR;
		data_d = GPIOD->IDR;
		
		DATA = ((data_e & 0xFF80) >> 3);
		DATA |= ((data_d & 0x0700) << 5);
		DATA |= ((data_d & 0xC000) >> 14);
		DATA |= ((data_d & 0x0003) << 2);	

		*(DB_data++) = DATA;





	}	
	
	delay_ns(2);
	
	cs1_Set;			
	
//	tick2=SysTick->VAL;	
	
//	raw_printf("diff_tick = %d\r\n", tick1 -tick2);
} 


/* 
* 名称：AD7606_read_data() 
* 功能：读取数据 
* 返回值：返回一个结构体指针，该指针为指向结构体数组的首地址  
*/ 

void Get_7606_Average(int16_t * vale)
{
	uint8_t  temp;
	u32 timout;

	timout = 10000;
	AD7606_startconvst();

	temp = BUSY_READ;			 // 读取 BUSY的状态 
	//while(temp == (uint8_t)Bit_SET)				//当busy为低电平时，数据转换完毕，此时可以读取数据 
	while(temp == GPIO_PIN_SET)
	{
//		IWDG_Feed();
		temp = BUSY_READ;			 // 读取 BUSY的状态 
		timout --;
		if (timout == 0)
		{
			printf("timeout\r\n");
			break;
		}
	}
	AD7606_read_data(vale); //读取数据放至数组DB_data[]
}
	
