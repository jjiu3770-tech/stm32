#ifndef __MY_MLDC4GA_XD_H__
#define __MY_MLDC4GA_XD_H__

#include "my_include.h"

//#include "stm32f10x.h"
//#include "usart.h"
//#include "delay.h"
//#include <string.h> 

#define USART_GSM  USART2

#define LED_GSM_RCC			RCC_APB2Periph_GPIOA
#define LED_GSM_GPIO		GPIOA
#define LED_GSM_GPIO_PIN	GPIO_Pin_12

#define led_gsm PAout(12)

//#define USARTSendString UART_SendString  
//#define USARTSendByte UART_SendByte   

#define NUM_SMS_MSG                     2 //内存中保留完整信息的个数
#define LENGTH_PAYLOAD                  32//完整信息的总长度
#define NUM_GPRS_MSG                    2 //内存中保留完整GPRS信息的个数

typedef struct 
{
    u16 length;//payload数据长度
    char telNum[14];
    char payload[LENGTH_PAYLOAD];
    u8 enable;
}_sms_msg_obj;



void My_MLDC4G_Init(void);
void My_MLDC4G_StateMachine(unsigned char msgByte);
void My_MLDC4G_ProcessMessage(void);
void OnGetSMSMessage(const _sms_msg_obj* smsMsgRec);
void My_MLDC4G_SendSMS(const char *telNum,const  char *sms);

#endif
