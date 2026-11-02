/******************************************************************************
    文件名:   flash_internal.c
    版本 :   v1.0
    作者  :   安诺科技 
    修改日期：20290608
    淘宝店连接：https://shop252056293.taobao.com/
******************************************************************************/
#include "main.h"
uint32_t g_flash_bankx = 0;     /* 记录操作哪个BANK */
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length);
/**
 * @brief       得到FLASH的错误状态
 * @param       无
 * @retval      执行结果
 *   @arg       0    : 已完成
 *   @arg       其他 : 错误编号
 */
static uint8_t stmflash_get_error_status(void)
{
    uint32_t res = 0;
    res = FLASH->SR;

    if (res & (1 << 16)) return 1;  /* BSY=1, 繁忙 */
    if (res & (1 << 15)) return 2;  /* OPTVERR=1,选项有效性错误 */
    if (res & (1 << 14)) return 3;  /* RDERR=1,读保护错误 */
    if (res & (1 << 9))  return 4;  /* FASTERR=1,快速编程错误 */
    if (res & (1 << 8))  return 5;  /* MISSERR=1,快速编程数据丢失错误 */
    if (res & (1 << 7))  return 6;  /* PGSERR=1,编程序列错误 */
    if (res & (1 << 6))  return 7;  /* SIZERR=1,数据大小错误 */
    if (res & (1 << 5))  return 8;  /* PGAERR=1,编程对齐错误 */
    if (res & (1 << 4))  return 9;  /* WRPERR=1,写保护错误 */
    if (res & (1 << 3))  return 10; /* PROGERR=1,编程错误 */
    if (res & (1 << 1))  return 11; /* OPERR=1,写/擦除错误 */

    return 0;                       /* 没有任何状态/操作完成. */
}

/**
 * @brief       等待操作完成
 * @param       time : 要延时的长短
 * @retval      执行结果
 *   @arg       0   : 已完成
 *   @arg       0XFF: 超时
 *   @arg       其他 : 错误编号
 */
static uint8_t stmflash_wait_done(uint32_t time)
{
    uint8_t res;

    do
    {
        res = stmflash_get_error_status();

        if (res != 1)
        {
            break;              /* 非忙, 无需等待了, 直接退出 */
        }
        
        time--;
    } while (time);

    if (time == 0)res = 0XFF;   /* 超时 */

    return res;
}
/**
 * @brief       在FLASH指定地址写两个字 (64bit数据)
 *   @note      这里写入两个字, 是指8个字节
 * @param       faddr   : 写入地址 (此地址必须为4的倍数!!)
 * @param       data    : 要写入的数据(32位)
 * @param       data2   : 要写入的数据(32位)
 * @retval      执行结果
 *   @arg       0    : 已完成
 *   @arg       0XFF : 超时
 *   @arg       其他 : 错误编号
 */
static uint8_t stmflash_write_double_word(uint32_t faddr, uint32_t data, uint32_t data2)
{
    uint8_t res;
    res = stmflash_wait_done(0XFFFF);
    if (res == 0)                                   /* OK */
    {
        FLASH->CR |= 1 << 0;                        /* 编程使能 */
        *(volatile uint32_t *)faddr = data;         /* 写入第一个字节数据 */
        *(volatile uint32_t *)(faddr + 4) = data2;  /* 写入第二个字节数据 */
        res = stmflash_wait_done(0XFFFF);           /* 等待操作完成,2个字编程 */

        if (res != 1)                               /* 操作成功 */
        {
            FLASH->CR &= ~(1 << 0);                 /* 清除PG位 */
        }
    }
    return res;
}

/**
 * @brief       从指定地址读取一个字 (32位数据)
 * @param       faddr   : 读取地址 (此地址必须为4倍数!!)
 * @retval      读取到的数据 (32位)
 */
uint32_t stmflash_read_word(uint32_t faddr)
{
    return *(volatile uint32_t *)faddr;
}
/**
 * @brief       获取某个地址所在的flash页
 * @param       addr    : lash地址
 * @retval      地址所在页
 */
uint32_t  stmflash_get_flash_page(uint32_t addr)
{
    uint32_t page = 0;

    if (addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
        /* Bank 1 */
        page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
        g_flash_bankx = FLASH_BANK_1;
    }
    else
    {
        /* Bank 2 */
        page = (addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
        g_flash_bankx = FLASH_BANK_2;
    }

    return page;
}
/**
  * @brief  This function does an erase of run app flash area
  * @param  start: start of run app flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
uint32_t FLASH_If_Erase(uint32_t u32EraseStartAddr,uint16_t u16ErasePageNum,uint16_t u16BankNum)
{  //擦除运行的APP处0x08022400-start
    FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError = 0;
	/* 解锁 */
	HAL_FLASH_Unlock();
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Page       = stmflash_get_flash_page(u32EraseStartAddr);;
	EraseInitStruct.Banks   = g_flash_bankx;
	EraseInitStruct.NbPages = u16BankNum; 
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
	{
		/*擦除出错，返回，实际应用中可加入处理 */
		return 1;
	}
	 HAL_FLASH_Lock();
	return (0);
}
/**
 * @brief       在FLASH 指定位置, 写入指定长度的数据(自动擦除)
 * @note        本函数写地址如果非0XFF那么会先擦除整个页区且不保存页区数据.所以写非0XFF的地址,将导致整个
 *              页区数据丢失.建议写之前确保页区里没有重要数据,最好是整个扇页区先擦除了,然后慢慢往后写.
 *              该函数对OTP区域也有效!可以用来写OTP区!
 *              OTP区域地址范围:0X1FFF7800~0X1FFF7A0F(注意：最后16字节，用于OTP数据块锁定，别乱写！！)
 * @param       waddr   : 起始地址 (此地址必须为4的倍数!!,否则写入出错!)
 * @param       pbuf    : 数据指针
 * @param       length  : 要写入的 字(32位)数(就是要写入的32位数据的个数)
 * @retval      无
 */
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    uint8_t status = 0;
    uint32_t addrx = 0;
    uint32_t endaddr = 0;
    
    if (waddr < STM32_FLASH_BASE || waddr % 4 ||        /* 写入地址小于 STM32_FLASH_BASE, 或不是4的整数倍, 非法. */
        waddr > (STM32_FLASH_BASE + STM32_FLASH_SIZE))  /* 写入地址大于 STM32_FLASH_BASE + STM32_FLASH_SIZE, 非法. */
    {
        return;
    }

    HAL_FLASH_Unlock();             /* 解锁 */
    FLASH->ACR &= ~(1 << 10);       /* FLASH擦除期间,必须禁止数据缓存!!! */
    FLASH->OPTR |= 1 << 22;         /* 使用双存储模式（一次写64bit也就是8字节） */
    addrx = waddr;                  /* 写入的起始地址 */
    endaddr = waddr + length * 4;   /* 写入的结束地址 */

    if (addrx < 0X1FFF0000)         /* 只有主存储区,才需要执行擦除操作!! */
    {
        while (addrx < endaddr)     /* 扫清一切障碍.(对非FFFFFFFF的地方,先擦除) */
        {
            if (stmflash_read_word(addrx) != 0XFFFFFFFF)    /* 有非0XFFFFFFFF的地方,要擦除这个扇区 */
            {
                EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;                                                    /* 选择页擦除 */
                EraseInitStruct.Page        = stmflash_get_flash_page(addrx);                                           /* 获取需要擦除哪个页 */
                EraseInitStruct.Banks       = g_flash_bankx;                                                            /* 需要擦除的页地址，所属BANK */
                EraseInitStruct.NbPages     = stmflash_get_flash_page(endaddr) - stmflash_get_flash_page(addrx) + 1;    /* 擦除的页数量 */
                if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)                                              
                {
                    break;
                }
                stmflash_wait_done(0XFFFF); 
            }
            else
            {
                addrx += 4;
            }
            status = stmflash_wait_done(0XFFFF);                                /* 等待上次操作完成 */
        }
    }
    status = stmflash_wait_done(0XFFFF);                                        /* 等待上次操作完成 */
    if (status == HAL_OK)
    {     
        while (waddr < endaddr)     /* 写数据 */
        {
            if(stmflash_write_double_word(waddr, *pbuf, *(pbuf + 1)))           /* 由于使用的double模式，所以一次写8字节，如果一次写4字节，会出现一次写成功一次写失败的情况 */
            {
                break;              /* 写入异常 */
            }

            waddr += 8;            
            pbuf += 2;             
        }
    }
    
    FLASH->ACR |= 1 << 10;          /* FLASH擦除结束,开启数据fetch */
    HAL_FLASH_Lock();               /* 上锁 */
}




