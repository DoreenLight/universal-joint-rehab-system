/******************************************************************************
    文件名:   mx25L1606E.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h" 

#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256

#define FLASH_CS_High()   	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET);
#define FLASH_CS_Low()     	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);
#define DUMMY_BYTE  0XFF
void FlashGet_Info(flashInfoTypedef * pFlashInfo);
void FlashGet_ElectronicInfo(flashInfoTypedef * pFlashInfo);

//-----------------------------------------------------------
//Description    : MX25L1606E初始化配置
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void MX25L1606E_Flash_Init(void)
{
	flashInfoTypedef flashInfo;   
	GPIO_InitTypeDef gpio_init_struct;
	 /*开启LED相关的GPIO外设时钟*/
    __HAL_RCC_GPIOB_CLK_ENABLE();
	//MX25L1606E芯片 CS控制脚
	gpio_init_struct.Pin = GPIO_PIN_6;
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init_struct.Pull = GPIO_PULLUP;
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &gpio_init_struct);	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);

	Flash_SPI_Init();                         						 //SPI1硬件接口配置
	Flash_SPI_SetSpeed(SPI_BAUDRATEPRESCALER_32);					 //设置为18M时钟,高速模式
	//读取Flash芯片ID号
	FlashGet_ElectronicInfo(&flashInfo);
	if((flashInfo.ManufacturerID==0x20)&&(flashInfo.DeviceID[0]==0x14))
	{ 

	}
	FlashGet_Info(&flashInfo);
	if((flashInfo.DeviceID[0]==0x40)&&(flashInfo.DeviceID[1]==0x16))
	{

	}   
}
//-----------------------------------------------------------
//Description    : 读取MX25L1606E写状态寄存器，判断是否写入完成
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void SPI_FLASH_WaitForWriteEnd(void)
{
    uint8_t FLASH_Status = 0;
    FLASH_CS_Low(); /* 选择 FLASH: CS 低 */
    Flash_SPI_ReadWriteByte(RDSD_CMD);/* 发送 读状态寄存器 命令 */
    do/* 若FLASH忙碌，则等待 */
    {	
        FLASH_Status = Flash_SPI_ReadWriteByte(DUMMY_BYTE);/* 读取FLASH芯片的状态寄存器 */	 
    }
    while ((FLASH_Status & 0x01) == SET);  /* 正在写入标志 */
    FLASH_CS_High(); /* 停止信号  FLASH: CS 高 */
}
//-----------------------------------------------------------
//Description    : 读取MX25L1606E芯片ID号
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashGet_Info(flashInfoTypedef * pFlashInfo)
{
//	    FlashWait_Busy();
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(RDID_CMD);
    pFlashInfo->ManufacturerID = Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    pFlashInfo->DeviceID[0] = Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    pFlashInfo->DeviceID[1] = Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    FLASH_CS_High();    
}
//-----------------------------------------------------------
//Description    : 读取MX25L1606E芯片制造商ID号
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashGet_ElectronicInfo(flashInfoTypedef * pFlashInfo)
{
//	    FlashWait_Busy();
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(REMS_CMD);
    Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    Flash_SPI_ReadWriteByte(0x00);  //manufacturer's ID first
    pFlashInfo->ManufacturerID = Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    pFlashInfo->DeviceID[0] = Flash_SPI_ReadWriteByte(DUMMY_BYTE);   
    FLASH_CS_High();    
}
//-----------------------------------------------------------
//Description    : 设置MX25L1606E进入休眠
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashEnter_DeepPowerDown(void)
{
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(DP_CMD);   
    FLASH_CS_High();    
}
//-----------------------------------------------------------
//Description    : 将MX25L1606E从休眠中唤醒
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashReleaseFrom_DeepPowerDown(void)
{
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(RDP_CMD);   
    FLASH_CS_High();    
}
uint8_t FlashReleaseFrom_DeepPowerDownE(void)
{   
    uint8_t ElectronicSignature;
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(RES_CMD);
    Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    ElectronicSignature = Flash_SPI_ReadWriteByte(DUMMY_BYTE);
    FLASH_CS_High();    
    return ElectronicSignature;
}
//-----------------------------------------------------------
//Description    : MX25L1606E写使能
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashWriteEnable(void)
{
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(WREN_CMD);   
    FLASH_CS_High();    
}
//-----------------------------------------------------------
//Description    : MX25L1606E写除能
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashWriteDisable(void)
{
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(WRDI_CMD);   
    FLASH_CS_High();    
}
//-----------------------------------------------------------
//Description    : MX25L1606E写状态寄器
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashWrite_StaRegister(uint8_t flashSta)
{
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(WRSR_CMD); 
    Flash_SPI_ReadWriteByte(flashSta); 
    FLASH_CS_High();    
}

//-----------------------------------------------------------
//Description    : 页写入函数
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashPage_Write(uint8_t *ptr,uint32_t pageAddr,uint32_t pageSize)
{
    uint32_t addr,i;
	  addr=pageAddr;
    FlashWriteEnable();
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(PP_CMD); 
    Flash_SPI_ReadWriteByte(addr>>16);
    Flash_SPI_ReadWriteByte(addr>>8);
    Flash_SPI_ReadWriteByte(addr); 
    for(i=0;i<pageSize;i++)
        Flash_SPI_ReadWriteByte(*ptr++); 
    FLASH_CS_High(); 

 /* 等待写入完毕*/
  SPI_FLASH_WaitForWriteEnd();
}
 /**
  * @brief  对FLASH写入数据，调用本函数写入数据前需要先擦除扇区
  * @param	pBuffer，要写入数据的指针
  * @param  WriteAddr，写入地址
  * @param  NumByteToWrite，写入数据长度
  * @retval 无
  */
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    /*mod运算求余，若writeAddr是SPI_FLASH_PageSize整数倍，运算结果Addr值为0*/
    Addr = WriteAddr % SPI_FLASH_PageSize;

    /*差count个数据值，刚好可以对齐到页地址*/
    count = SPI_FLASH_PageSize - Addr;
    /*计算出要写多少整数页*/
    NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
    /*mod运算求余，计算出剩余不满一页的字节数*/
    NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

    /* Addr=0,则WriteAddr 刚好按页对齐 aligned  */
    if (Addr == 0)
    {
        /* NumByteToWrite < SPI_FLASH_PageSize */
        if (NumOfPage == 0) 
        {
            FlashPage_Write(pBuffer, WriteAddr, NumByteToWrite);
        }
        else /* NumByteToWrite > SPI_FLASH_PageSize */
        { 
            /*先把整数页都写了*/
            while (NumOfPage--)
            {
                FlashPage_Write(pBuffer, WriteAddr, SPI_FLASH_PageSize);
                WriteAddr +=  SPI_FLASH_PageSize;
                pBuffer += SPI_FLASH_PageSize;
            }
            /*若有多余的不满一页的数据，把它写完*/
            FlashPage_Write(pBuffer, WriteAddr, NumOfSingle);
        }
    }
    /* 若地址与 SPI_FLASH_PageSize 不对齐  */
    else 
    {
        /* NumByteToWrite < SPI_FLASH_PageSize */
        if (NumOfPage == 0)
        {
            /*当前页剩余的count个位置比NumOfSingle小，一页写不完*/
            if (NumOfSingle > count) 
            {
                temp = NumOfSingle - count;
                /*先写满当前页*/
                FlashPage_Write(pBuffer, WriteAddr, count);

                WriteAddr +=  count;
                pBuffer += count;
                /*再写剩余的数据*/
                FlashPage_Write(pBuffer, WriteAddr, temp);
            }
            else /*当前页剩余的count个位置能写完NumOfSingle个数据*/
            {
                FlashPage_Write(pBuffer, WriteAddr, NumByteToWrite);
            }
        }
        else /* NumByteToWrite > SPI_FLASH_PageSize */
        {
            /*地址不对齐多出的count分开处理，不加入这个运算*/
            NumByteToWrite -= count;
            NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
            NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

            /* 先写完count个数据，为的是让下一次要写的地址对齐 */
            FlashPage_Write(pBuffer, WriteAddr, count);

            /* 接下来就重复地址对齐的情况 */
            WriteAddr +=  count;
            pBuffer += count;
            /*把整数页都写了*/
            while (NumOfPage--)
            {
                FlashPage_Write(pBuffer, WriteAddr, SPI_FLASH_PageSize);
                WriteAddr +=  SPI_FLASH_PageSize;
                pBuffer += SPI_FLASH_PageSize;
            }
            /*若有多余的不满一页的数据，把它写完*/
            if (NumOfSingle != 0)
            {
                FlashPage_Write(pBuffer, WriteAddr, NumOfSingle);
            }
        }
    }
}
//-----------------------------------------------------------
//Description    : 整个芯片擦除
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashChip_Erase(void)
{
    SPI_FLASH_WaitForWriteEnd();             /* 等待写入完毕*/
    FlashWriteEnable();
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(CE_CMD);     
    FLASH_CS_High(); 
    SPI_FLASH_WaitForWriteEnd();             /* 等待擦除完毕*/
}
//-----------------------------------------------------------
//Description    : 块擦除 一次性擦除一块大小为64K
//Input          : None
//Output         : None
//Author         : C7 20190612
//Note(s)        : None
//-----------------------------------------------------------
void FlashBlock_Erase(uint8_t blockAddr)
{
    uint32_t addr;//,i;  
    addr=((uint32_t)blockAddr)<<16;//每个block有16个扇区，每个扇区有4KByte;
    SPI_FLASH_WaitForWriteEnd();  /* 等待空闲*/
    FlashWriteEnable();
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(BE_CMD); 
    Flash_SPI_ReadWriteByte(addr>>16);
    Flash_SPI_ReadWriteByte(addr>>8);
    Flash_SPI_ReadWriteByte(addr); 
    FLASH_CS_High();   
    SPI_FLASH_WaitForWriteEnd();             /* 等待擦除完毕*/
}

//------------------------------------------------------------------------------
//Description    : 按块擦除
//Input          : None
//Output         : None
//Author         : C7
//Note(s)        : None
//------------------------------------------------------------------------------
uint8_t W25Q64_64K_Blockerase(uint16_t sect_no, uint8_t flgwait) 
{
    uint32_t addr;	
    FlashWriteEnable();
    addr = sect_no;
    addr = addr << 16;      //  64K 为  1 0000 0000 0000 0000  所以需要左移16位 
    FLASH_CS_Low(); 
    Flash_SPI_ReadWriteByte(CMD_BLOCK_ERASE64KB); 
    Flash_SPI_ReadWriteByte(addr >> 16);
    Flash_SPI_ReadWriteByte(addr>>8);
    Flash_SPI_ReadWriteByte(addr); 
    FLASH_CS_High(); 
    SPI_FLASH_WaitForWriteEnd();
    return 0;
}
//------------------------------------------------------------------------------
//Description    : 扇形擦除
//Input          : None
//Output         : None
//Author         : C7
//Note(s)        : None
//------------------------------------------------------------------------------
void FlashSector_Erase(uint32_t sectorAddr)
{
    uint32_t addr;//,i;
    addr=((uint32_t)sectorAddr)/*<<12*/;   //每个扇形大小为4K，地址移4K
    SPI_FLASH_WaitForWriteEnd();  /* 等待空闲*/
    FlashWriteEnable();
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(SE_CMD); 
    Flash_SPI_ReadWriteByte(addr>>16);
    Flash_SPI_ReadWriteByte(addr>>8);
    Flash_SPI_ReadWriteByte(addr); 
    FLASH_CS_High(); 
    SPI_FLASH_WaitForWriteEnd();             /* 等待擦除完毕*/
}
/*-------------------------------------------------------------------------------------
  * @brief  读取FLASH数据 
  * @param 	ptr，存储读出数据的指针
  * @param   addr，读取地址
  * @param   len，读取数据长度  (不定长度)
  * @retval 无
------------------------------------------------------------------------------------------*/
void FlashRead(uint8_t *ptr,uint32_t addr,uint32_t len)
{
    uint32_t addr_tmp,i;
    addr_tmp=addr; 
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(READ_CMD); 
    Flash_SPI_ReadWriteByte(addr_tmp>>16);
    Flash_SPI_ReadWriteByte(addr_tmp>>8);
    Flash_SPI_ReadWriteByte(addr_tmp); 
    for(i=0;i<len;i++)
        *ptr++=Flash_SPI_ReadWriteByte(DUMMY_BYTE); 
    FLASH_CS_High();    
}
/*------------------------------------------------------------------------------------------
  * @brief  读取FLASH单页数据
  * @param 	ptr，存储读出数据的指针
  * @param   pageAddr，读取地址
  * @param   pageSize，读取数据长度  单页数据长度256
  * @retval 无
------------------------------------------------------------------------------------------*/
void FlashPage_Read(uint8_t *ptr,uint32_t pageAddr,uint32_t pageSize)
{
    uint32_t addr,i;
      addr=pageAddr;
    addr=((uint32_t)addr)<<8;
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(READ_CMD); 
    Flash_SPI_ReadWriteByte(addr>>16);
    Flash_SPI_ReadWriteByte(addr>>8);
    Flash_SPI_ReadWriteByte(addr); 
    for(i=0;i<pageSize;i++)
        *ptr++=Flash_SPI_ReadWriteByte(DUMMY_BYTE); 
    FLASH_CS_High();    
}

void FlashFastRead(uint32_t addr,uint8_t *ptr,uint32_t len)
{
    uint32_t addr_tmp,i;
    addr_tmp=addr&0x001fffff;
    FLASH_CS_Low();
    Flash_SPI_ReadWriteByte(FREAD_CMD); 
    Flash_SPI_ReadWriteByte(addr_tmp>>16);
    Flash_SPI_ReadWriteByte(addr_tmp>>8);
    Flash_SPI_ReadWriteByte(addr_tmp); 
    Flash_SPI_ReadWriteByte(DUMMY_BYTE); 
    for(i=0;i<len;i++)
        *ptr++=Flash_SPI_ReadWriteByte(DUMMY_BYTE); 
    FLASH_CS_High();    
}
//------------------------------------------------------------------------------
//Description    : 擦除主机程序存储A区  
//Input          : None
//Output         : None
//Author         : C7
//Note(s)        : None
//------------------------------------------------------------------------------
void EraseMasterBinFileBlockA(void)
{
	uint16_t i;

	for(i = 0;i < 8;i++)			
	{
        W25Q64_64K_Blockerase(i,1);	
	}   
}

//------------------------------------------------------------------------------
//Description    : 擦除主机程序存储B区  
//Input          : None
//Output         : None
//Author         : C7
//Note(s)        : None
//------------------------------------------------------------------------------
void EraseMasterBinFileBlockB(void)
{
    uint16_t i;

	for(i = 8;i < 16;i++)			    
	{
        W25Q64_64K_Blockerase(i,1);	
	}  
}
//------------------------------------------------------------------------------
//Description    : 擦除从机1程序存储区   
//Input          : None
//Output         : None
//Author         : C7
//Note(s)        : None
//------------------------------------------------------------------------------
void EraseSlave1BinFileBlock(void)
{
	uint16_t i;
	for(i = 16;i < 24;i++)			   
	{
        W25Q64_64K_Blockerase(i,1);	
	}  
}
//------------------------------------------------------------------------------
//Description    : 擦除从机2程序存储区   
//Input          : None
//Output         : None
//Author         : C7
//Note(s)        : None
//------------------------------------------------------------------------------
void EraseSlave2BinFileBlock(void)
{
	uint16_t i;
	for(i = 24;i < 32;i++)			   
	{
        W25Q64_64K_Blockerase(i,1);	
	}  
}
