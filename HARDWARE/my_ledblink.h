/****************************************************************
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//TWKJ 32开发板
//通旺科技@TWKJ
//作者：tianqingyong
//版本：V2.1
//修改日期:2020/07/19
//程序功能：封装和简化LED闪烁或蜂鸣器鸣叫一定次数的操作
//V1.0  2020/03/26  完成基本功能
//V2.0  2020/06/30  修改为可多个pin设备同时操作
                    以管脚号作为唯一识别标志
                    增加设置次数和停止工作的函数
//V2.1  2020/07/19  1、My_LEDBlink函数增加控制管脚初始化代码
****************************************************************/
#ifndef __MY_LED_BLINK_H
#define __MY_LED_BLINK_H
#include "my_include.h"

#define NUM_LED_Blink               5 //
#define My_LEDBlink_Stop(n)         My_LEDBlink_SetTimes(n,0)

void My_LEDBlink(MyPinDef _pin,u8 volt_ledON,u16 times,u16 time_on,u16 time_off);
void My_LEDBlinkProcess(void);
void My_LEDBlink_SetTimes(MyPinDef _pin,u16 times);
#endif
