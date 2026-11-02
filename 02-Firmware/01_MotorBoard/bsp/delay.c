#include "delay.h"
#include "main.h"

static u32 fac_us = 0;                      // us延时倍乘数

// 使用DWT实现高精度延时的初始化
static void DWT_Init(void)
{
    // 检查是否支持DWT
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    {
        // 使能DWT
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    
    // 使能CYCCNT计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // 清零CYCCNT计数器
    DWT->CYCCNT = 0;
}

// 初始化延迟函数
void delay_init(void)
{
    // 获取系统时钟频率
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    
    // 计算us延时的倍乘系数
    fac_us = sysclk / 1000000;  // 每微秒的CPU周期数
    
    // 初始化DWT
    DWT_Init();
}

// 延时n微秒
void delay_us(u32 nus)
{
    uint32_t ticks = nus * fac_us;
    uint32_t start = DWT->CYCCNT;
    
    while ((DWT->CYCCNT - start) < ticks)
    {
        // 等待
    }
}

// 延时n毫秒
void delay_ms(u16 nms)
{
    while (nms--)
    {
        delay_us(1000);  // 延时1ms
    }
}

// 延时n纳秒（精度有限）
void delay_ns(uint32_t ns)
{
    uint32_t cycles;
    
    if (fac_us == 0)
    {
        delay_init();  // 如果没有初始化，先初始化
    }
    
    // 计算需要的CPU周期数
    cycles = (ns * fac_us) / 1000;
    
    if (cycles < 4)
    {
        cycles = 4;
    }
    
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles)
    {
        // 等待
    }
}
