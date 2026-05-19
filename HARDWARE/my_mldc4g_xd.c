#include "my_mldc4g_xd.h"



/********************************************************
***************模块初始化  ml307r-dc4G 2026 ***************
********************************************************/
void My_MLDC4G_Init(void)
{
    unsigned char i ;

	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LED_GSM_RCC, ENABLE);	 //使能PA端口时钟
    
	GPIO_InitStructure.GPIO_Pin = LED_GSM_GPIO_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(LED_GSM_GPIO, &GPIO_InitStructure);	  
    
    led_gsm = 0;//短信指示灯控制
//	for(i=0;i<20;i++){delay_ms(500); }  //分段延时7s    
	for(i=0;i<20;i++)
    {
//    USARTSendString(USART_GSM , "ATE0\r\n");   //设置字符集
        USARTSendString(USART_GSM , "ATE1\r\n");   //设置字符集
        delay_ms(500);          //延时有助于稳定
    }  

	USARTSendString(USART_GSM , "AT+CMGF=1\r\n");   //设置字符集
	delay_ms(50);          //延时有助于稳定

	USARTSendString(USART_GSM , "AT+CSMP=33,167,0,0\r\n"); //设置干什么、
	delay_ms(100);          //延时有助于稳定

	USARTSendString(USART_GSM , "AT+CMGD=1,4\r\n"); //删除所有短信
	delay_ms(50);          //延时有助于稳定	 
    led_gsm = 1;//短信指示灯控制   
}

void My_MLDC4G_SendSMS(const char *telNum,const  char *sms)//telNum格式8618105140357
{
    unsigned char i ;
    
    led_gsm = 0;//短信指示灯控制
    
	USARTSendString(USART_GSM , "AT+CMGF=1\r\n");   //设置字符集
	delay_ms(50);          //延时有助于稳定

	USARTSendString(USART_GSM , "AT+CSMP=33,167,0,0\r\n"); //设置干什么、
	delay_ms(100);          //延时有助于稳定
    
    USARTSendString(USART_GSM,"AT+CMGS=\"+");//发送短信AT命令头
    USARTSendString(USART_GSM,(char *)telNum);      //要发送的手机号码
    USARTSendString(USART_GSM,"\"\r\n");    //
    delay_ms(20);

    USARTSendString(USART_GSM,(char *)sms);//发送具体内容
//	delay_ms(20);
//	USARTSendByte(USART_GSM,0x1a); //发送
	delay_ms(20);
    USARTSendByte(USART_GSM,0x1a); //发送
    
	for(i=0;i<25;i++) delay_ms(100);          //延时有助于稳定
    led_gsm = 1;//短信指示灯控制 
}

#define GSM_MES_BUF_SIZE 256 //单条AT命令及返回数据 的最大数据长度

u8 gsmRecClear = 0; //短信接收定时清零
u8 gsmRecCount = 0; //短信接计数
u8 readGsmIng = 0;  //可以去读取短信
char gsmRecBuf[GSM_MES_BUF_SIZE];//短信接收定时清零

void My_MLDC4G_StateMachine(u8 msgByte)//接收状态机
{
    gsmRecClear=0;      //空闲定时读取 清零
    gsmRecBuf[gsmRecCount++]=msgByte;//记录接收数据
    if(gsmRecCount>=(GSM_MES_BUF_SIZE-1))                  //连续接收16个字符信息
    {
        gsmRecCount=0;
        readGsmIng=1;               //接收完成标志位置1
    }

}

#define DELAY_TIMES 10 //AT读取命令间隔
unsigned char readGsmStep  =0;	//运行步骤
unsigned int readGsmDelay =0;//接收短信处理延时

_sms_msg_obj sms_msg[1];//结构体,模块接收到的短信数据

void My_MLDC4G_ProcessMessage(void)//gsm程序处理
{
    unsigned char i;
    char *midStr;//中间变量指针    
    unsigned char getGsmTelFlag= 0;//提取到短信号码标志
    
    static unsigned char delGsmMesCount = 0;//删除短信计数处理
    
    delay_ms(5);//延时处理
    if(readGsmDelay > 0){readGsmDelay--;}//延时处理
    else
    {
        switch (readGsmStep)        
        {
            case 0:
                USARTSendString(USART_GSM ,"AT+CMGF=1\r\n"); //设置字符集
                readGsmDelay = DELAY_TIMES*2;//延时一定时间在处理下一个命令 202405
                readGsmStep++;//指向下一个命令
                break;
            case 1:
				for(i=0;i<GSM_MES_BUF_SIZE-1;i++)
				{gsmRecBuf[i]=0; }  //清空timebuf
				gsmRecCount = 0;//为接收短信数据做准备			
                
                USARTSendString(USART_GSM ,"AT+CMGR=1\r\n"); //设置字符集
                readGsmDelay = DELAY_TIMES*15;//此处时间不易过短 否则数据还没有接收完 202405
                readGsmStep++;//指向下一个命令
                break;    
            case 2:                
                
                for(i=0;i<14;i++)sms_msg[0].telNum[i]=0; //清空号码
                for(i=0;i<LENGTH_PAYLOAD;i++)sms_msg[0].payload[i]=0;//清空数据体
                                               
                midStr = strstr((const char*)gsmRecBuf,",\"+86");//在接收中查看是否有+86
                if(midStr != NULL)//接收到短信
                {
                    midStr++;midStr++;midStr++;//+号不保存 在发送短信时就已经添加了
                    for(i=0;i<13;i++)sms_msg[0].telNum[i]=*midStr++; //提取短信号码  
                    getGsmTelFlag = 1; //提取到短信号码                   
                }
                midStr = strstr((const char*)gsmRecBuf,"READ\",\"1");//在接收中查看是否有+86
                if(midStr != NULL)//接收到短信
                {   
                    midStr = midStr + strlen("READ\",\"1")-1;
                    sms_msg[0].telNum[0] = '8';
                    sms_msg[0].telNum[1] = '6';
                    for(i=0;i<11;i++)sms_msg[0].telNum[2+i]=*midStr++; //提取短信号码                
                    getGsmTelFlag = 1; //提取到短信号码
                }              
                if(getGsmTelFlag != 0)//提取到手机号 提取短信内容 需要在删除前提取防止数据覆盖
                {         
                    led_gsm = 0;//短信指示灯控制
                    midStr = strstr((const char*)gsmRecBuf,"\"\r\n");//在接收中查看是否有短信内容处+CMGR: "REC UNREAD","18105140357","","13/10/25,15:12:59+32"
                    if(midStr != NULL)
                    {                           
                        for(i=0;i<LENGTH_PAYLOAD;i++)
                        {
                            if(*midStr == '\r' && *(midStr+1) == '\n' && *(midStr+2) == 'O'&& *(midStr+3) == 'K')
                            {break;}
                            sms_msg[0].payload[i]=*midStr++; //提取短信号码
                        }                                                                
                    }                
                }

                delGsmMesCount++;//多次读取后清空一次短信
                if(delGsmMesCount >= 10 || getGsmTelFlag != 0)
                {
                    delGsmMesCount = 0;//清空短信 
                    USARTSendString(USART_GSM ,"AT+CMGD=1,4\r\n"); //把该短信删除
                    if(getGsmTelFlag != 0)
                    {
                        delay_ms(200);delay_ms(200);delay_ms(200);//延时处理                                       
                        delay_ms(200);delay_ms(200);delay_ms(200);                     
//                        delay_ms(200);delay_ms(200);delay_ms(200); 
//                        delay_ms(200);delay_ms(200);delay_ms(200);                     
//                        delay_ms(200);delay_ms(200);delay_ms(200);                         
                    }
                    delay_ms(100); 
                }
                
                if(getGsmTelFlag !=0)//提取到号码后
                {
                    OnGetSMSMessage(&sms_msg[0]); 
                    
                    getGsmTelFlag = 0;//清楚所有接收短信标志
                    for(i=0;i<GSM_MES_BUF_SIZE-1;i++)
                    {gsmRecBuf[i]=0; }  //清空timebuf 
                    
                    led_gsm = 1;//短信指示灯控制
                }                               
                readGsmStep++;//指向下一个命令         
                readGsmDelay = DELAY_TIMES*15;//延时一定时间在处理下一个命令  202405               
                
//                USARTSendString(USART_GSM , "\r\n");
//                USARTSendString(USART_GSM , sms_msg[0].telNum);
//                USARTSendString(USART_GSM , "\r\n");
//                USARTSendString(USART_GSM , sms_msg[0].payload);
//                USARTSendString(USART_GSM , "\r\n");
                break;                        
            default:readGsmStep=0;break;
        }
    }
}


void __weak OnGetSMSMessage(const _sms_msg_obj* smsMsgRec)
{
    
}

/********************************************************
***************模块初始化  ml307r-dc4G 2026 ***************
********************************************************/

