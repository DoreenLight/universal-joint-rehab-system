/******************************************************************************
    文件名:   Flash_spi.h
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#ifndef _flash_spi_H
#define _flash_spi_H
#include "main.h"

extern void Flash_SPI_Init(void);			 //初始化SPI口
extern void Flash_SPI_SetSpeed(uint8_t SPI_BaudRatePrescaler);
extern uint8_t Flash_SPI_ReadWriteByte(uint8_t TxData);//SPI总线读写一个字节 
#endif

