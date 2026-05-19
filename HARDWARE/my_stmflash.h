/************************************************
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//TWKJ STM32开发板
//通旺科技@TWKJ
//作者：tianqingyong
//版本：V2.2
//修改日期:2020/10/23
//程序功能：封装和简化STM32内部Flash数据读写的操作
//V1.1  2020/04/08  优化程序
//V2.0  2020/07/07  将自定义的扇区存储程序改为支持多组数据
//V2.1  2020/09/14  优化自定义的扇区存储程序，改为无需关注数据存储在哪个扇区
//V2.2  2020/10/23  1、优化读写程序
************************************************/
#ifndef __MY_FLASH_H__
#define __MY_FLASH_H__
#include "my_include.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
#define FLASH_BASE_ADDR         FLASH_BASE//在stm32f10x.h中有定义
//用户根据自己的需要设置
#define STM32_FLASH_SIZE        64//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN        1//使能FLASH写入(0，不是能;1，使能)
#define FLASH_WAITETIME         50000//FLASH等待超时时间
#define NUM_SAVE_SECTOR         3//定义“扇区保存”的数据种类数（见本文件中的最后三个函数）
//////////////////////////////////////////////////////////////////////////////////////////////////////

#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE         1024 //字节
#else 
#define STM_SECTOR_SIZE         2048
#endif
#define FLASH_SECTOR_COUNT      STM32_FLASH_SIZE/STM_SECTOR_SIZE
#ifndef	FLASH_FLAG_WRPRTERR
#define FLASH_FLAG_WRPRTERR     FLASH_FLAG_WRPERR
#endif

#if	defined (USE_HAL_DRIVER) 
extern void    FLASH_PageErase(uint32_t PageAddress);
#endif

//读取指定地址的半字(16位数据)
//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u8 My_STMFlash_ReadByte(u32 addr);
u16 My_STMFlash_ReadHalfWord(u32 addr);
u32 My_STMFlash_ReadWord(u32 faddr);
void My_STMFlash_ReadBytes(u32 ReadAddr,u8 *pBuffer,u16 length);
void My_STMFlash_ReadHalfWords(u32 ReadAddr,u16 *pBuffer,u16 length);
void My_STMFlash_ReadWords(u32 ReadAddr,u32 *pBuffer,u16 length);
#ifdef STM32_FLASH_WREN	//如果使能了写  
u32 My_STMFlash_WriteBytes_NoCheck(u32 WriteAddr,u8 *pBuffer,u16 length);
u32 My_STMFlash_WriteHalfWord_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 length);
u32 My_STMFlash_WriteWord_NoCheck(u32 WriteAddr,u32 *pBuffer,u16 length);
void My_STMFlash_Write(u32 WriteAddr,u16 *pBuffer,u16 length);
#endif
/***************************************************************************************
以下程序功能为将若干个数据保存到一个Flash扇区中，一般用于保存变化频率较高的数据
如电动车仪表保存里程，,若0.1公里保存一次，按照一般方法，保存一次就擦除一次，则只能保存最大10000公里
而使用此方法，以1扇区1024字节为例，保存0.1公里的数据是6个字节，则最大可保存到1700000公里
该方法的基本原理是将数据在同一个扇区按照地址顺序依次保存，存满后再擦除，然后从扇区起始地址继续存储
***************************************************************************************/
bool My_STMFlash_SaveUseSector(void *saveData);
//读取使用整个扇区保存若干个数据的数据内容，返回值代表Flash扇区中有没有保存数据
bool My_STMFlash_ReadSectorSave(void *saveData);
/*****************************************************************************************
初始化使用整个扇区保存若干个数据，并读取保存的数据，返回值代表Flash扇区中有没有保存数据
sectorAddr:保存数据的扇区地址
saveData：需要保存的数据指针，可以是任意类型的数据指针，在读写时都会转换为uint数据类型
byteCount：需要保存的数据的总字节数
*****************************************************************************************/
bool My_STMFlash_SectorSaveInit(u32 sectorAddr, void *buffer, u16 byteCount);
u16 My_STMFlash_GetVersion(void);
#endif

