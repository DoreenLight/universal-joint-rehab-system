/******************************************************************************
    文件名:   main.h
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#ifndef _main_H_
#define _main_H_

#include "string.h"
#include <stdio.h>
#include <stdint.h>
#include "delay.h"           
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Timer.h"
#include "Timer_pro.h"
#include "crc16.h"
#include "flash_internal.h"
#include "usart1.h"
#include "usart3.h"
#include "usart4.h"
#include "extern_device_control.h"
#include "mx25L1606E.h"
#include "Flash_spi.h"
#define DF_TASK1_TIMER      1     //1ms
#define DF_TASK2_TIMER      5    //10ms
#define DF_TASK3_TIMER      10   //25ms
#define DF_TASK4_TIMER      25   //50ms
#define DF_TASK5_TIMER      50   //100ms
#define DF_TASK6_TIMER      100  //100ms

#define DF_Task_NUM   6

#define DF_GET_UPGRADE_MSG_STEP       0x00
#define DF_DOWNLOAD_FIRMWARE_STEP     0x01
#define DF_PRO_UPGRADE_MSG_STEP       0x02
#define DF_GOTO_APP_RUN_STEP          0x03


#define DF_UART1_COM 0x01
#define DF_UART2_COM 0x02
#define DF_UART3_COM 0x03

extern uint8_t u8Gu8TaskDelayTimeTab[DF_Task_NUM];                //任务运行时间管理
extern uint8_t u8Gu8TaskDelayRunTab[DF_Task_NUM];
extern void Error_Handler(void);




#endif

