/******************************************************************************
    文件名:   flash_internal.h
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#ifndef _Flash_internal_H
#define _Flash_internal_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* FLASH起始地址 */
#define STM32_FLASH_BASE        0x08000000      /* STM32 FLASH 起始地址 */
#define STM32_FLASH_SIZE        0x080000        /* STM32 FLASH 总大小 */
#define FLASH_WAITETIME         50000           /* FLASH等待超时时间 */
extern uint32_t FLASH_If_Erase(uint32_t u32EraseStartAddr,uint16_t u16ErasePageNum,uint16_t u16BankNum);
uint32_t stmflash_read_word(uint32_t faddr);                                        /* 读出字 */
void stmflash_write(uint32_t WriteAddr, uint32_t *pBuffer, uint32_t NumToWrite);    /* 从指定地址开始写入指定长度的数据 */
void stmflash_read(uint32_t ReadAddr, uint32_t *pBuffer, uint32_t NumToRead);       /* 从指定地址开始读出指定长度的数据 */
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length);



#endif 
