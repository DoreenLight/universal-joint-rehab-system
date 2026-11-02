#ifndef _AD7606_H
#define _AD7606_H
#include "stm32f4xx_hal.h" 

#include "main.h"



//#define sampling_0times 0 
//#define sampling_2times 1 
//#define sampling_4times 2 
//#define sampling_8times 3 
//#define sampling_16times 4 
//#define sampling_32times 5 
//#define sampling_64times 6

#define OS10_Set 	  			HAL_GPIO_WritePin(AD7606_OS1_GPIO_Port,AD7606_OS1_Pin,GPIO_PIN_SET);
#define OS10_Reset  			HAL_GPIO_WritePin(AD7606_OS1_GPIO_Port,AD7606_OS1_Pin,GPIO_PIN_RESET);

#define OS11_Set 	  			HAL_GPIO_WritePin(AD7606_OS1_GPIO_Port,AD7606_OS1_Pin,GPIO_PIN_SET);
#define OS11_Reset  			HAL_GPIO_WritePin(AD7606_OS1_GPIO_Port,AD7606_OS1_Pin,GPIO_PIN_RESET);

#define OS12_Set 	  			HAL_GPIO_WritePin(AD7606_OS2_GPIO_Port,AD7606_OS2_Pin,GPIO_PIN_SET);	 
#define OS12_Reset  			HAL_GPIO_WritePin(AD7606_OS2_GPIO_Port,AD7606_OS2_Pin,GPIO_PIN_RESET);

//#define SERI_Set 	  			GPIO_SetBits(GPIOG,GPIO_Pin_12);	 
//#define SERI_Reset  			GPIO_ResetBits(GPIOG,GPIO_Pin_12);

//#define STby_Set 	 		GPIO_SetBits(GPIOA,GPIO_Pin_3);	 //外部上拉到3.3V

#define convstA_Set 	  	HAL_GPIO_WritePin(AD7606_CONVST_GPIO_Port,AD7606_CONVST_Pin,GPIO_PIN_SET);	 		
#define convstA_Reset   	HAL_GPIO_WritePin(AD7606_CONVST_GPIO_Port,AD7606_CONVST_Pin,GPIO_PIN_RESET);

#define convstB_Set 	  	HAL_GPIO_WritePin(AD7606_CONVST_GPIO_Port,AD7606_CONVST_Pin,GPIO_PIN_SET);	 	
#define convstB_Reset   	HAL_GPIO_WritePin(AD7606_CONVST_GPIO_Port,AD7606_CONVST_Pin,GPIO_PIN_RESET);

#define rst_Set 	 		HAL_GPIO_WritePin(AD7606_RESET_GPIO_Port,AD7606_RESET_Pin,GPIO_PIN_SET);	
#define rst_Reset  		  	HAL_GPIO_WritePin(AD7606_RESET_GPIO_Port,AD7606_RESET_Pin,GPIO_PIN_RESET);	

#define rd_Set 	  			HAL_GPIO_WritePin(AD7606_RD_GPIO_Port,AD7606_RD_Pin,GPIO_PIN_SET);		
#define rd_Reset   			HAL_GPIO_WritePin(AD7606_RD_GPIO_Port,AD7606_RD_Pin,GPIO_PIN_RESET);		

#define cs1_Set 	  		HAL_GPIO_WritePin(AD7606_CS_GPIO_Port,AD7606_CS_Pin,GPIO_PIN_SET);		
#define cs1_Reset   		HAL_GPIO_WritePin(AD7606_CS_GPIO_Port,AD7606_CS_Pin,GPIO_PIN_RESET);	

#define busy_Set 	  		HAL_GPIO_WritePin(AD7606_BUSY_GPIO_Port,AD7606_BUSY_Pin);		
#define busy_Reset   		HAL_GPIO_WritePin(AD7606_BUSY_GPIO_Port,AD7606_BUSY_Pin);	

#define range_10V 	 		HAL_GPIO_WritePin(AD7606_RANGE_GPIO_Port,AD7606_RANGE_Pin,GPIO_PIN_SET);	
#define range_5V  		  	HAL_GPIO_WritePin(AD7606_RANGE_GPIO_Port,AD7606_RANGE_Pin,GPIO_PIN_RESET);	

//#define frstdata_Set 	  	GPIO_SetBits(GPIOA,GPIO_Pin_7);		
//#define frstdata_Reset  	GPIO_ResetBits(GPIOA,GPIO_Pin_7);	//这个应该是输入引脚
#define BUSY_READ       	HAL_GPIO_ReadPin(AD7606_BUSY_GPIO_Port,AD7606_BUSY_Pin)	

//#define BUSY_READ       	GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_4)	

// 定义有符号16位整数类型
typedef int16_t s16;

extern void GPIO_AD7606_Configuration(void);
extern void AD7606_Init(void);
extern void AD7606_startconvst(void);
extern void AD7606_reset(void);
extern void AD7606_read_data(int16_t * DB_data);
extern void Get_7606_Average(int16_t * vale);

float adc2volt(unsigned int adc);

float adc2g(unsigned int adc);

#endif

//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
