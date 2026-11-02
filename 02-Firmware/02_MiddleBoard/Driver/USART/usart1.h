/******************************************************************************
    文件名:   uart1.1
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#ifndef _USART1_H
#define _USART1_H
#include "main.h"
#define DF_UART1_BAUD_RARE   115200                  					//串口1波特率
#define DF_UART1_REC_BUF_LEN 1048                     					//串口1收发缓冲区长度
void Uart1Init(uint32_t u32baud);
extern void Uart1DMASendFunction(uint8_t *u8Buf, uint16_t u16len);
void Uart1RxToUart4Task(void);
#endif 
