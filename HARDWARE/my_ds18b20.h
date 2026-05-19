/************************************************
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//TWKJ STM32开发板
//通旺科技@TWKJ
//作者：tianqingyong
//版本：V1.0
//修改日期:2019/10/30
//程序功能：封装和简化DS18B20的操作,C文件和头文件无须做任何修改
************************************************/
#ifndef __MY_DS18B20_H
#define __MY_DS18B20_H 
#include "my_include.h"   

u8 My_DS18B20_Init(MyPinDef pin);//初始化DS18B20
float My_DS18B20_GetTemp(MyPinDef pin);	//获取温度  
#endif















