/************************************************
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//TWKJ STM32开发板
//通旺科技@TWKJ
//作者：tianqingyong
//版本：V1.2
//修改日期:2020/12/30
//程序功能：封装和简化ADC的初始化等操作，适用于标准库
//V1.0  2019/10/25  1、完成基本功能
//V1.1  2020/12/23  1、增加DMA方式采集
//                  2、增加获取GPIO管脚对应的ADC通道
//V1.2  2020/12/30  1、优化My_ADC_GetPercent函数
************************************************/ 
 #include "my_adc.h"
 
#define VERSION         12

//初始化ADC
void  My_ADC_PreInit(ADC_TypeDef* ADCx,u32 trigger,bool enableDMA)
{
    u8 i;
    ADC_InitTypeDef ADC_InitStructure; 

    if(ADCx==ADC1)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);//使能ADC1通道时钟
    }
    else if(ADCx==ADC2)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);//使能ADC1通道时钟
    }
    else if(ADCx==ADC3)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);//使能ADC1通道时钟
    }

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M

    for(i=0;i<ArrayCount(Pins_ADC);i++)
    {
        GPIO_Pin_Init(Pins_ADC[i],GPIO_Mode_AIN);//初始化为模拟通道输入引脚 
    }

    ADC_DeInit(ADCx);  //复位ADC1,将外设 ADC1 的全部寄存器重设为缺省值

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;//ADC工作模式:ADC1和ADC2工作在独立模式
    ADC_InitStructure.ADC_ScanConvMode = enableDMA?ENABLE:DISABLE;//模数转换工作在单通道模式
    ADC_InitStructure.ADC_ContinuousConvMode = enableDMA?ENABLE:DISABLE;//模数转换工作在单次转换模式
    ADC_InitStructure.ADC_ExternalTrigConv = trigger;//ADC_ExternalTrigConv_None;//转换由软件而不是外部触发启动
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//ADC数据右对齐
    ADC_InitStructure.ADC_NbrOfChannel = ArrayCount(Pins_ADC);//顺序进行规则转换的ADC通道的数目
    ADC_Init(ADCx, &ADC_InitStructure);//根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   

    if(enableDMA)
    {
        u8 i;
        for(i=0;i<ArrayCount(Pins_ADC);i++)
        {
            ADC_RegularChannelConfig(ADCx, My_ADC_GetADCChannel(ADCx,Pins_ADC[i]), i+1, ADC_SampleTime_239Cycles5 );//ADC1,ADC通道,采样时间为239.5周期
        }
        /* Enable ADC1 DMA */
        ADC_DMACmd(ADCx, ENABLE);
    }
    
    ADC_Cmd(ADCx, ENABLE);//使能指定的ADC1
    ADC_ResetCalibration(ADCx);//使能复位校准
    while(ADC_GetResetCalibrationStatus(ADCx));//等待复位校准结束
    ADC_StartCalibration(ADCx);//开启AD校准
    while(ADC_GetCalibrationStatus(ADCx));//等待校准结束

//    ADC_SoftwareStartConvCmd(ADCx, ENABLE);//使能指定的ADC的软件转换启动功能
}
void  My_ADC_Init(ADC_TypeDef* ADCx)
{
    My_ADC_PreInit(ADCx,ADC_ExternalTrigConv_None,false);
}
#ifdef __MY_DMA_H
/************************************************
函数功能：ADC采集DMA方式初始化
ADCx：ADC转换器选择（ADC通道在头文件中定义）
adcValue：存储ADC转换后的结果（二位数组，下标为转换次数和通道总数）
adcDataNumber：采集的数据总数（半字计数）
************************************************/ 
void  My_ADC_Init_DMA(ADC_TypeDef* ADCx,u16 *adcValue,u16 adcDataNumber)
{
    My_ADC_PreInit(ADCx,ADC_ExternalTrigConv_None,true);
    My_DMA_Init(DMA1_Channel1,DMA_DIR_PeripheralSRC,(uint32_t)&(ADC1->DR),(uint32_t)adcValue,adcDataNumber,DATA_SIZE_HALFWORD);
}
/************************************************
函数功能：启动一次DMA方式的ADC采集
ADCx：ADC转换器选择（ADC通道在头文件中定义）
adcDataNumber：采集的数据总数（半字计数）
************************************************/ 
void My_ADC_StartADC_DMA(ADC_TypeDef* ADCx,u16 adcDataNumber)
{
    DMA_SetCurrDataCounter(DMA1_Channel1,adcDataNumber);//设置DMA的传送数量
    DMA_Cmd(DMA1_Channel1,ENABLE);
    ADC_SoftwareStartConvCmd(ADCx, ENABLE);//使用软件转换启动功能
}
/************************************************
函数功能：检查DMA方式的ADC采集是否完成
ADCx：ADC转换器选择（ADC通道在头文件中定义）
return：完成则返回true，否则返回false
************************************************/ 
bool My_ADC_DMAComplete(ADC_TypeDef* ADCx)
{
    if(DMA_GetFlagStatus(DMA1_FLAG_TC1)!=RESET)//DMA传送完成
    {
        DMA_ClearFlag(DMA1_FLAG_TC1);//清除DMA传送完成标志
        DMA_Cmd(DMA1_Channel1,DISABLE);
        ADC_SoftwareStartConvCmd(ADCx, DISABLE);
        return true;
    }
    return false;
}
#endif
/************************************************
函数功能：查询GPIO管脚对应的ADC通道
ADCx：ADC转换器选择（ADC通道在头文件中定义）
pin：GPIO管脚
return：是ADC管脚则返回对应的ADC通道，否则返回0xff
************************************************/ 
u8 My_ADC_GetADCChannel(ADC_TypeDef* ADCx,MyPinDef pin)
{
    if(ADCx==ADC1 || ADCx==ADC2 || ADCx==ADC3)
    {
        if(pin>=PC0 && pin<=PC3)
        {
            return (ADC_Channel_10 + pin - PC0);
        }
        if(pin<=PA3)
        {
            return (ADC_Channel_0 + pin - PA0);
        }
        if(ADCx==ADC3)
        {
            if(pin>=PF6 && pin<=PF10)
            {
                return (ADC_Channel_4 + pin - PF6);
            }
        }
        else
        {
            if(pin>=PA4 && pin<=PA7)
            {
                return (ADC_Channel_4 + pin - PA4);
            }
            if(pin==PC4 || pin==PC5)
            {
                return (ADC_Channel_14 + pin - PC4);
            }
            if(pin==PB0 || pin==PB1)
            {
                return (ADC_Channel_8 + pin - PB0);
            }
        }
    }
    return 0xff;
}
//获得ADC值
//ch:通道值 0~3
u16 My_ADC_GetValue(ADC_TypeDef* ADCx,u8 ch)
{
    //设置指定ADC的规则组通道，一个序列，采样时间
    ADC_RegularChannelConfig(ADCx, ch, 1, ADC_SampleTime_239Cycles5 );//ADC1,ADC通道,采样时间为239.5周期
    ADC_SoftwareStartConvCmd(ADCx, ENABLE);//使能指定的ADC1的软件转换启动功能
    while(!ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC ));//等待转换结束
    return ADC_GetConversionValue(ADCx);//返回最近一次ADC1规则组的转换结果
}

u16 My_ADC_GetAverage(ADC_TypeDef* ADCx,u8 ch,u8 times)
{
    u32 temp_val=0;
    u8 t;
    for(t=0;t<times;t++)
    {
        temp_val+=My_ADC_GetValue(ADCx,ch);
    }
    return temp_val/times;
}

u8 My_ADC_GetPercent(ADC_TypeDef* ADCx,u8 ch,u8 times,float adcVlaueMin,float adcValueMax)
{
    u16 percent = 0;
    percent = My_ADC_GetAverage(ADCx,ch,times);
    //烟雾变化数值范围是ADC_SMOKE_LOW-ADC_SMOKE_HIGH
    if(percent<adcVlaueMin)//
    {
        return 0;
    }
    if(percent>adcValueMax)
    {
        return 100;
    }
    else
    {
        //将adc采集的数值转换为0-100的相对浓度数值
        percent = (percent-adcVlaueMin)*100/(adcValueMax-adcVlaueMin);
    }
    return percent;
}

float My_ADC_GetCurrent_ASC712(ADC_TypeDef* ADCx,u8 ch,u8 times,float scale)
{
    float volt = My_ADC_GetVoltage(ADCx,ch,times,scale);
    if(volt<Volt_ACS712_ZERO)//电压值对比
    {
        volt = 0;
    }
    else
    {
        volt = (volt-Volt_ACS712_ZERO)/0.185;
    }//正常情况下计算比例 
    return volt;
}
u16 My_ADC_GetVersion(void)
{
    return VERSION;
}

