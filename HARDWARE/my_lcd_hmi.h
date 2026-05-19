
#ifndef __MY_LCD_HMI_H
#define __MY_LCD_HMI_H
#include "my_include.h"

/************************************
自定义协议：
定义      起始字节    数据内容    结束字节
标识      start       data        end
示例      0x2a        0x50 0x78   0x23
***************************************/
#define COUNT_MSG_LCD_HMI_MAX                      2//
#define LEN_MSG_LCD_HMI_MAX                        32//信息容量
#define LCD_MSG_START                               0x2a//串口发送数据起始高字节
#define LCD_MSG_END                                 0x23//串口发送数据起始低字节
/****************************************************************************************/


#define My_LCD_HMIMessage_Receive                  My_LcdHmiMessage_StateMachine


typedef struct 
{
    bool enable:1;
    u16 length:15;
    char payload[LEN_MSG_LCD_HMI_MAX];
}_lcdHmi_msg_obj;



void My_LcdHmi_Init(USART_TypeDef* USARTx,u32 baud);
void My_LcdHmiMessage_StateMachine(u8 msgByte);
void My_LcdHmiMessage_Process(void);
void OnGetLcdHmiMessage(const _lcdHmi_msg_obj *lcdHmiMsgRec);
#endif
