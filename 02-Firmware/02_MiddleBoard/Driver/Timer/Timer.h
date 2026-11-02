/******************************************************************************
    文件名:   Timer.h
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#ifndef _TIMER_H_
#define _TIMER_H_
#include "main.h"
/******************************************************************************************/

/* 通用定时器 定义 */
#define GTIM_TIMX_INT                       TIM3
#define GTIM_TIMX_INT_IRQn                  TIM3_IRQn
#define GTIM_TIMX_INT_IRQHandler            TIM3_IRQHandler
#define GTIM_TIMX_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)      /* TIM3 时钟使能 */

/******************************************************************************************/

void TIM3_Init(uint16_t arr, uint16_t psc);                                        /* 通用定时器 定时中断初始化函数 */


#endif

