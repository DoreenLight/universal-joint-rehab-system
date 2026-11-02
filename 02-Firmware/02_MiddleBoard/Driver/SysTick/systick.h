/*********************************************************************************************************
*
* File                : systick.h
* Hardware Environment: 
* Build Environment   : RealView MDK-ARM  Version: 4.20
* Version             : V1.0
* By                  : 
*
*                                  (c) Copyright 2005-2011, CKS
*                                       http://www.cksic.com
*                                          All Rights Reserved
*
*********************************************************************************************************/

#ifndef __SYSTICK_H
#define __SYSTICK_H 

/* Includes ------------------------------------------------------------------*/	   
#include "stm32f10x.h"

/* Private function prototypes -----------------------------------------------*/

extern void delay_init(void);
extern void delay_ms(u16 nms);
extern void delay_us(u32 nus);
extern void delay_s(u16 ns);

#endif
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/






























