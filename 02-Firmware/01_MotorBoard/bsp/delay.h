#ifndef __DELAY_H
#define __DELAY_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

// 函数声明
void delay_init(void);                      // 初始化延时函数
void delay_us(u32 nus);                     // 延时n微秒
void delay_ms(u16 nms);                     // 延时n毫秒
void delay_ns(uint32_t ns);                 // 延时n纳秒（有限精度）

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */
