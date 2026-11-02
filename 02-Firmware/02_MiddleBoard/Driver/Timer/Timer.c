/******************************************************************************
    文件名:   Timer.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h" 
TIM_HandleTypeDef g_timx_handle; /* 定时器x句柄 */
//-----------------------------------------------------------
//Description    : 定时器3初始化
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void TIM3_Init(uint16_t u16Arr,uint16_t u16Psc)
{
   GTIM_TIMX_INT_CLK_ENABLE();                                 /* 使能TIMx时钟 */

    g_timx_handle.Instance = GTIM_TIMX_INT;                     /* 通用定时器x */
    g_timx_handle.Init.Prescaler = u16Psc;                         /* 预分频系数 */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;        /* 递增计数模式 */
    g_timx_handle.Init.Period = u16Arr;                            /* 自动装载值 */
    HAL_TIM_Base_Init(&g_timx_handle);

    HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 1, 3);             /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);                     /* 开启TIMx中断 */

    HAL_TIM_Base_Start_IT(&g_timx_handle);                      /* 使能定时器x和更新中断 */	 
}

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
	 static uint8_t u8TastNumberCount=0;   
    /* 以下代码没有使用定时器HAL库共用处理函数来处理，而是直接通过判断中断标志位的方式 */
    if(__HAL_TIM_GET_FLAG(&g_timx_handle, TIM_FLAG_UPDATE) != RESET)
    {
        /*分时处理延时
        *****************************************************/	
        for(u8TastNumberCount=0;u8TastNumberCount<DF_Task_NUM;u8TastNumberCount++)
        {
            if(u8Gu8TaskDelayTimeTab[u8TastNumberCount])
            {
                u8Gu8TaskDelayTimeTab[u8TastNumberCount]--;
                if(u8Gu8TaskDelayTimeTab[u8TastNumberCount]==0)
                {
                    u8Gu8TaskDelayRunTab[u8TastNumberCount]=0x01;
                }
            }
        }
        __HAL_TIM_CLEAR_IT(&g_timx_handle, TIM_IT_UPDATE);      /* 清除定时器溢出中断标志位 */
    }
}

