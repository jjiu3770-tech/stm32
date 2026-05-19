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
#include "my_ledblink.h"

typedef struct
{
    u32 ticks;//系统定时器计数值
    u16 times;
    u16 time_on;
    u16 time_off;
    MyPinDef pin;
    u8 blinkState;
    bool enable;
    u8 volt_led_on;
}_LED_Blink_obj;

_LED_Blink_obj ledBlink[NUM_LED_Blink];//控制参数结构体数组

/****************************************************************
函数功能：LED闪烁程序初始化
_pin：LED控制管脚
volt_ledON：LED点亮时控制管脚输出的电平
times：LED闪烁次数
time_on：每一次闪烁LED点亮的时间，单位：毫秒
time_off：每一次闪烁LED熄灭的时间，单位：毫秒
****************************************************************/
void My_LEDBlink(MyPinDef _pin,u8 volt_ledON,u16 times,u16 time_on,u16 time_off)
{
    u8 i;
    for(i=0;i<ArrayCount(ledBlink);i++)
    {
        if(ledBlink[i].pin==_pin)
        {
            break;
        }
    }
    if(i==ArrayCount(ledBlink))
    {
        for(i=0;i<ArrayCount(ledBlink);i++)
        {
            if(ledBlink[i].enable == false)
            {
                break;
            }
        }
    }
    ledBlink[i].pin = _pin;//控制管脚
    ledBlink[i].volt_led_on = volt_ledON;//器件工作时的电平
    ledBlink[i].times = times;//器件控制的次数
    ledBlink[i].time_on = time_on;//器件工作的时间
    ledBlink[i].time_off = time_off;//期间不工作的时间
    ledBlink[i].enable = true;
    GPIO_Pin_Init(_pin,GPIO_Mode_Out_PP);//初始化控制管脚
}
/****************************************************************
函数功能：处理蜂鸣器鸣叫或LED闪烁事件处理（状态机）
****************************************************************/
void My_LEDBlinkProcess(void)
{
    u8 i = 0;//
    for(i=0;i<ArrayCount(ledBlink);i++)
    {
        if(ledBlink[i].enable == true)
        {
            if(ledBlink[i].times>0)//如果该器件控制次数大于0
            {
                if(ledBlink[i].blinkState == 0)
                {
                    ledBlink[i].blinkState = 1;
                }
                switch(ledBlink[i].blinkState)
                {
                    case 1:
                        //根据设置的器件工作电平控制器件
                        PinOut(ledBlink[i].pin) = ledBlink[i].volt_led_on?1:0;
                        ledBlink[i].ticks = My_SysTick_GetTicks();//记录系统时间
                        ledBlink[i].blinkState++;//指向下一个状态
                        break;
                    case 2:
                        if(ledBlink[i].time_on<=((My_SysTick_GetTicks()-ledBlink[i].ticks)*My_SysTick_GetPeriod()))//开启时间到达
                        {
                            PinOut(ledBlink[i].pin) = ledBlink[i].volt_led_on?0:1;
                            ledBlink[i].ticks = My_SysTick_GetTicks();//记录系统时间
                            ledBlink[i].blinkState++;//指向下一个状态
                        }
                        break;
                    case 3:
                        if(ledBlink[i].time_off<=((My_SysTick_GetTicks()-ledBlink[i].ticks)*My_SysTick_GetPeriod()))//关闭时间到达
                        {
                            ledBlink[i].times--;//该器件控制次数-1
                            ledBlink[i].blinkState=1;//回到第一个处理状态
                        }
                        break;
                        default:break;
                }
            }
            else//本次器件控制事件结束
            {
                PinOut(ledBlink[i].pin) = ledBlink[i].volt_led_on?0:1;
                ledBlink[i].blinkState = 0;//回到初始状态
                ledBlink[i].enable = false;
            }
        }
    }
}
/****************************************************************
函数功能：设置指定控制管脚的LED闪烁的次数
****************************************************************/
void My_LEDBlink_SetTimes(MyPinDef _pin,u16 times)
{
    u8 i;
    for(i=0;i<ArrayCount(ledBlink);i++)
    {
        if(ledBlink[i].pin==_pin)
        {
            ledBlink[i].times = times;//器件控制的次数
            break;
        }
    }
}
