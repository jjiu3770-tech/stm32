
#include "my_lcd_hmi.h"

_lcdHmi_msg_obj lcdHmiMsg[COUNT_MSG_LCD_HMI_MAX];//结构体 


USART_TypeDef* LCD_HMI_UARTx;

//初始化串口屏 接口及波特率
void My_LcdHmi_Init(USART_TypeDef* USARTx,u32 baud)
{
    My_USART_Init(USARTx,baud);
    LCD_HMI_UARTx = USARTx;//记录串口号
}

///****************************************************************
//函数功能：发送一个字符串显示
//****************************************************************/
//void My_LcdHmiSend_txt(char* txtNamestr,char* txt)
//{
//    My_USART_printf(LCD_HMI_UARTx,"%s=\"%s\"\xff\xff\xff",txtNamestr,txt);
//}

/****************************************************************
函数功能：数据接收状态机，在串口中断调用
msgByte：串口接收到的字节数据
****************************************************************/
void My_LcdHmiMessage_StateMachine(u8 msgByte)
{
    static u8 index_dat=0;
    static u8 index_msg_read=0;
    static u8 startRecFlag = 0;    
    
    if (msgByte == '*')
    {
        startRecFlag = 1;//启动接收
        index_dat = 0;
        lcdHmiMsg[index_msg_read].payload[index_dat++] = msgByte;   //存储接收到的数据
    }
    else if(startRecFlag == 1)
    {    
        lcdHmiMsg[index_msg_read].payload[index_dat++] = msgByte; //存储接收到的数据
        if(msgByte  == '#')    
        {
            lcdHmiMsg[index_msg_read].enable = true;//标志数据可以交给应用程序处理
            index_msg_read++;//指向下一个消息体
            if(index_msg_read>=COUNT_MSG_LCD_HMI_MAX) //接收过多组数据 重新计数
            {
                index_msg_read=0;
            }
            index_dat = 0;//清空数据重新接收
            startRecFlag =0;
        } 
        else if(index_dat>=LEN_MSG_LCD_HMI_MAX)//单独数据不能超过定义长度   
        {
            startRecFlag = 0;//清空数据重新接收
            index_dat = 0;        
        }        
    }
}
/****************************************************************
函数功能：处理接收的数据，在主程序中调用
****************************************************************/
void My_LcdHmiMessage_Process(void)
{
    static u8 index_msg_process=0;
    u8 i;

    if(lcdHmiMsg[index_msg_process].enable == true)//如果接收到了一个完整的数据
    {
        OnGetLcdHmiMessage(&lcdHmiMsg[index_msg_process]);
        lcdHmiMsg[index_msg_process].enable = false;
        memset(lcdHmiMsg[index_msg_process].payload,0,lcdHmiMsg[index_msg_process].length);
        //指向下一条信息
        index_msg_process++;
        if(index_msg_process==COUNT_MSG_LCD_HMI_MAX)
        {
            index_msg_process=0;
        }
    }
}
/****************************************************************
函数功能：信息接收处理回调函数
此处为弱定义空函数，开发者需自行重定义实现
recMsgPtr：接收到的信息的指针
****************************************************************/
void __weak OnGetLcdHmiMessage(const _lcdHmi_msg_obj *lcdHmiMsgRec)
{

}

