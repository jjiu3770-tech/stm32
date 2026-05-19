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
#include "my_stmflash.h"

#define VERSION         22
 
// STM32F030F4P6 16KB FLASH,4KB RAM,1个扇区1KB,程序起始0x8000000
// STM32F103C8T6 64KB FLASH,20KB RAM,1个扇区1KB,程序起始0x8000000 
//flash擦除时是整个山区擦除
//读取指定地址的半字(16位数据)
//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u8 My_STMFlash_ReadByte(u32 addr)
{
    return *(__IO u8*)addr; 
}
u16 My_STMFlash_ReadHalfWord(u32 addr)
{
    return *(__IO u16*)addr; 
}
u32 My_STMFlash_ReadWord(u32 faddr)
{
    return *(__IO uint32_t *)faddr; 
}

void My_STMFlash_ReadBytes(u32 ReadAddr,u8 *pBuffer,u16 length)
{
    while(length--)
    {
        *pBuffer++ = *(__IO u8*)ReadAddr;//读取1个字节.
        ReadAddr++;//偏移1个字节.
    }
}
//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//length:半字(16位)数
void My_STMFlash_ReadHalfWords(u32 ReadAddr,u16 *pBuffer,u16 length)
{
    while(length--)
    {
        *pBuffer++ = *(__IO u16*)ReadAddr;//读取2个字节.
        ReadAddr+=2;//偏移2个字节.
    }
}
void My_STMFlash_ReadWords(u32 ReadAddr,u32 *pBuffer,u16 length)
{
    while(length--)
    {
        *pBuffer++ = *(__IO u32*)ReadAddr;//读取4个字节.
        ReadAddr+=4;//偏移4个字节.
    }
}
#ifdef STM32_FLASH_WREN//如果使能了写
//不检查的写入
//WriteAddr:起始地址
//pBuffer:数据指针
//length:半字(16位)数
u32 My_STMFlash_WriteBytes_NoCheck(u32 WriteAddr,u8 *pBuffer,u16 length)
{
    u16 i,*writeBufPtr;
    u32 realWriteAddr = WriteAddr;
    if(WriteAddr&0x01)
    {
        if(*(u16 *)(WriteAddr-1)!=0xffff)
        {
            WriteAddr++;
            realWriteAddr = WriteAddr;
        }
        else
        {
        #if !defined (USE_HAL_DRIVER)
            FLASH_ProgramHalfWord(WriteAddr-1,(pBuffer[0]<<8)|0xff);
        #else
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,WriteAddr-1,(pBuffer[0]<<8)|0xff);
        #endif
            pBuffer++;
            length--;
            WriteAddr++;
        }
    }
    writeBufPtr = (u16 *)pBuffer;
    i = length>>1;
    while(i--)
    {
    #if !defined (USE_HAL_DRIVER)
        FLASH_ProgramHalfWord(WriteAddr,*writeBufPtr++);
    #else
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,WriteAddr,*writeBufPtr++);
    #endif
        WriteAddr+=2;//地址增加2.
    }
    if(length&0x01)
    {
        i = *(pBuffer+length-1);
    #if !defined (USE_HAL_DRIVER)
        FLASH_ProgramHalfWord(WriteAddr,0xff00|i);
    #else
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,WriteAddr,0xff00|i);
    #endif
    }
    return realWriteAddr;
}
u32 My_STMFlash_WriteHalfWord_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 length)   
{
    u16 i;
    u32 realWriteAddr;
    if(WriteAddr&0x01)
    {
        WriteAddr++;
    }
    realWriteAddr = WriteAddr;
    for(i=0;i<length;i++)
    {
    #if !defined (USE_HAL_DRIVER)
        FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
    #else
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,WriteAddr,pBuffer[i]);
    #endif
        WriteAddr+=2;//地址增加2.
    }
    return realWriteAddr;
}
u32 My_STMFlash_WriteWord_NoCheck(u32 WriteAddr,u32 *pBuffer,u16 length)
{
    u16 i;
    u32 realWriteAddr;
    if(WriteAddr&0x03)
    {
        WriteAddr += 4 - (WriteAddr&0x03);
    }
    realWriteAddr = WriteAddr;
    for(i=0;i<length;i++)
    {
    #if !defined (USE_HAL_DRIVER)
        FLASH_ProgramWord(WriteAddr,pBuffer[i]);
    #else
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,pBuffer[i]);
    #endif
        WriteAddr+=4;//地址增加4.
    }
    return realWriteAddr;
}
//从指定地址开始写入指定长度的数据
//WriteAddr:起始地址(此地址必须为2的倍数!!)
//pBuffer:数据指针
//length:半字(16位)数(就是要写入的16位数据的个数.)
u16 buffer_flash[STM_SECTOR_SIZE/2];//最多是2K字节
void My_STMFlash_Write(u32 WriteAddr,u16 *pBuffer,u16 length)
{
    u16 addr_offset_sector;//扇区内偏移地址(16位字计算)
    u16 countCanWrite; //扇区内剩余地址(16位字计算)
    u16 i;
    if(WriteAddr<FLASH_BASE||(WriteAddr>=(FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//非法地址
#if !defined (USE_HAL_DRIVER)
        FLASH_Unlock();//解锁
#else
        HAL_FLASH_Unlock();//解锁
#endif
    addr_offset_sector = WriteAddr%STM_SECTOR_SIZE;//在扇区内的偏移(2个字节为基本单位.)
    while(length>0) 
    {
        countCanWrite=(STM_SECTOR_SIZE-addr_offset_sector)/2;//扇区剩余空间大小 
        if(length<=countCanWrite)
        {
            countCanWrite=length;//不大于该扇区范围
        }
        My_STMFlash_ReadHalfWords(WriteAddr-addr_offset_sector,buffer_flash,STM_SECTOR_SIZE/2);//读出整个扇区的内容
        for(i=0;i<countCanWrite;i++)//校验数据
        {
            if(buffer_flash[addr_offset_sector/2+i]!=0XFFFF)break;//需要擦除    
        }
        if(i<countCanWrite)//需要擦除
        {
#if !defined (USE_HAL_DRIVER)
            FLASH_ErasePage(WriteAddr-addr_offset_sector);//擦除这个扇区
#else
            FLASH_PageErase(WriteAddr-addr_offset_sector);//擦除这个扇区
            FLASH_WaitForLastOperation(FLASH_WAITETIME);//等待上次操作完成
            CLEAR_BIT(FLASH->CR, FLASH_CR_PER);//清除CR寄存器的PER位，此操作应该在FLASH_PageErase()中完成！
                                                                        //但是HAL库里面并没有做，应该是HAL库bug！
#endif
            for(i=0;i<countCanWrite;i++)//复制
            {
                buffer_flash[i+(addr_offset_sector>>1)]=pBuffer[i];
            }
            My_STMFlash_WriteHalfWord_NoCheck(WriteAddr-addr_offset_sector,buffer_flash,STM_SECTOR_SIZE/2);//写入整个扇区  
        }
        else 
        {
            My_STMFlash_WriteHalfWord_NoCheck(WriteAddr,pBuffer,countCanWrite);//写已经擦除了的,直接写入扇区剩余区间.
        }  
        addr_offset_sector=0;//偏移位置为0
        pBuffer += (countCanWrite*2);//指针偏移
        WriteAddr += (countCanWrite*2);//写地址偏移
        length-=countCanWrite;//字节(16位)数递减
    }
#if !defined (USE_HAL_DRIVER)
    FLASH_Lock();//上锁
#else
    HAL_FLASH_Lock();//上锁
#endif
}

/***************************************************************************************
以下程序功能为将若干个数据保存到一个Flash扇区中，一般用于保存变化频率较高的数据
如电动车仪表保存里程，,若0.1公里保存一次，按照一般方法，保存一次就擦除一次，则只能保存最大10000公里
而使用此方法，以1扇区1024字节为例，保存0.1公里的数据是6个字节，则最大可保存到1700000公里
该方法的基本原理是将数据在同一个扇区按照地址顺序依次保存，存满后再擦除，然后从扇区起始地址继续存储
NUM_SAVE_SECTOR：初始化的保存数据的区域个数（扇区数）
***************************************************************************************/
static u16 addrOffset_sectorSave[NUM_SAVE_SECTOR];//计算实际需要保存数据的偏移地址
static u16 *dataPtr[NUM_SAVE_SECTOR];//需要保存的数据的指针
static u16 dataSizeToSave[NUM_SAVE_SECTOR];//需要保存的16位数据长度
static u32 addr_sector[NUM_SAVE_SECTOR];//保存数据的扇区地址
/**************************************************************************************
读取使用整个扇区保存若干个数据的数据内容，返回值代表保存有没有成功
saveData:保存数据的扇区地址（即初始化时传入的地址），数据字节数已在初始化时保存
**************************************************************************************/
bool My_STMFlash_SaveUseSector(void *saveData)
{
    u8 index_sector;
    for(index_sector=0;index_sector<NUM_SAVE_SECTOR;index_sector++)
    {
        if(dataPtr[index_sector]==saveData)
        {
            break;
        }
    }
    if(index_sector>=NUM_SAVE_SECTOR)
    {
        return false;
    }
    My_STMFlash_WriteHalfWord_NoCheck(addr_sector[index_sector]+addrOffset_sectorSave[index_sector], dataPtr[index_sector],dataSizeToSave[index_sector]);
    addrOffset_sectorSave[index_sector]+=dataSizeToSave[index_sector]<<1;
    if((STM_SECTOR_SIZE-addrOffset_sectorSave[index_sector])<(dataSizeToSave[index_sector]<<1)){//扇区写满
#if !defined (USE_HAL_DRIVER)
        FLASH_ErasePage(addr_sector[index_sector]);//擦除整个扇区
#else
        FLASH_PageErase(addr_sector[index_sector]);//擦除这个扇区
        FLASH_WaitForLastOperation(FLASH_WAITETIME);//等待上次操作完成
        CLEAR_BIT(FLASH->CR, FLASH_CR_PER);//清除CR寄存器的PER位，此操作应该在FLASH_PageErase()中完成！
                                           //但是HAL库里面并没有做，应该是HAL库bug！
#endif
        addrOffset_sectorSave[index_sector]=0;
        return My_STMFlash_SaveUseSector(saveData);
    }
    return true;
}
/*****************************************************************************************
读取使用整个扇区保存若干个数据的数据内容，返回值代表Flash扇区中有没有保存数据
saveData:保存数据的扇区地址（即初始化时传入的地址），数据字节数已在初始化时保存
*****************************************************************************************/
bool My_STMFlash_ReadSectorSave(void *saveData)
{
    u16 index_t;//计算保存了多少次，进而计算下次要保存的地址
    u16 buffer;
    u8 index_sector;
    for(index_sector=0;index_sector<NUM_SAVE_SECTOR;index_sector++)
    {
        if(dataPtr[index_sector]==saveData)
        {
            break;
        }
    }
    if(index_sector>=NUM_SAVE_SECTOR)
    {
        return false;
    }
    //计算保存次数，同时将结果加1保存
    for(index_t=STM_SECTOR_SIZE/2;index_t>0;index_t--)
    {
        buffer = My_STMFlash_ReadHalfWord(addr_sector[index_sector]+((index_t-1)<<1));
        if(buffer!=0xffff)
        {
            break;
        }
    }
    if(index_t==0)//整个扇区被擦除(第一次上电)
    {
        addrOffset_sectorSave[index_sector] = 0;
#if !defined (USE_HAL_DRIVER)
        FLASH_Unlock();//解锁
#else
        HAL_FLASH_Unlock();//解锁
#endif
        return false;
    }
    else{
        addrOffset_sectorSave[index_sector] = ((index_t-1)/dataSizeToSave[index_sector])*dataSizeToSave[index_sector]<<1;
        for(index_t=0;index_t<dataSizeToSave[index_sector];index_t++)
        {
            *(dataPtr[index_sector]+index_t) = My_STMFlash_ReadHalfWord(addr_sector[index_sector]+addrOffset_sectorSave[index_sector]);
            addrOffset_sectorSave[index_sector]+=2;
        }
#if !defined (USE_HAL_DRIVER)
        FLASH_Unlock();//解锁
#else
        HAL_FLASH_Unlock();//解锁
#endif
        return true;
    }
}
/*****************************************************************************************
初始化使用整个扇区保存若干个数据，并读取保存的数据，返回值代表Flash扇区中有没有保存数据
sectorAddr:保存数据的扇区地址
saveData：需要保存的数据指针，可以是任意类型的数据指针，在读写时都会转换为uint数据类型
byteCount：需要保存的数据的总字节数
*****************************************************************************************/
bool My_STMFlash_SectorSaveInit(u32 sectorAddr, void *saveData, u16 byteCount)
{
    u8 index;
    for(index=0;index<NUM_SAVE_SECTOR;index++)
    {
        if(dataPtr[index]==NULL)
        {
            break;
        }
    }
    if(index>=NUM_SAVE_SECTOR)
    {
        return false;
    }
    addr_sector[index] = sectorAddr - sectorAddr%STM_SECTOR_SIZE;
    dataPtr[index] = saveData;
    dataSizeToSave[index] = (byteCount>>1) + (byteCount&0x01);
    return My_STMFlash_ReadSectorSave(saveData);
}

#endif
u16 My_STMFlash_GetVersion(void)
{
    return VERSION;
}

