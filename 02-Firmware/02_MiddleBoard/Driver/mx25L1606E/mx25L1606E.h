/******************************************************************************
    文件名:   mx25L1606E.h
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#ifndef MX25L1606E_H
#define MX25L1606E_H
#include "main.h"

#define FLASH_BUSY_TIMEOUT  200

//#define FLASH_PAGE_SIZE     256

#define RDID_CMD  0X9F
#define RDSD_CMD  0X05
#define WRSR_CMD  0x01
#define REMS_CMD  0X90
#define DP_CMD    0XB9
#define RDP_CMD   0XAB
#define RES_CMD   0XAB
#define PP_CMD    0X02
#define WREN_CMD  0X06
#define WRDI_CMD  0X04
#define CE_CMD    0XC7
#define BE_CMD    0X52
#define SE_CMD    0x20
#define READ_CMD  0X03
#define FREAD_CMD 0X0B
#define DREAD_CMD 0X3B
#define CMD_BLOCK_ERASE64KB       0xd8


typedef struct
{
    uint8_t ManufacturerID;
    uint8_t DeviceID[2];
}flashInfoTypedef;
void MX25L1606E_Flash_Init(void);
void FlashPage_Read(uint8_t *ptr,uint32_t pageAddr,uint32_t pageSize);
void FlashPage_Write(uint8_t *ptr,uint32_t pageAddr,uint32_t pageSize);
void FlashSector_Erase(uint32_t sectorAddr);
void FlashBlock_Erase(uint8_t blockAddr);
void FlashChip_Erase(void);
extern void FlashRead(uint8_t *ptr,uint32_t addr,uint32_t len);
extern void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
extern void EraseMasterBinFileBlockA(void);
extern void EraseMasterBinFileBlockB(void);
extern void EraseSlave1BinFileBlock(void);
extern void EraseSlave2BinFileBlock(void);
#endif
