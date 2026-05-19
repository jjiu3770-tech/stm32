#include "my_include.h"

#define F_SIZE      12 //定义显示字体大小 汉字显示要定义了字模才可以
#define MyLCD_Show(m,n,p)     LCD_ShowString(LCD_GetPos_X(F_SIZE,m),LCD_GetPos_Y(16,n),p,F_SIZE,false)  //显示函数 
u16 yPlace = 0; //显示位置y坐标值

void scanKeyAnddealKey(void);
void My_ESP8266_SendStrStr(USART_TypeDef* USARTx, const char *str);
unsigned char makeSureLinkCount=0; //确保链接变量
void displayOfCollectedData(void);//显示采集到的数据

#define ADDR_FLASH_WRITE            (FLASH_BASE_ADDR+STM32_FLASH_SIZE*1024-STM_SECTOR_SIZE*1)
int16 saveData[10] = {60,0,3,32,0,2,10,0,0,10};

#define lset  saveData[0]//设置阈值
#define lmode saveData[1]//当前模式
#define ldw   saveData[2]//档位
#define tset  saveData[3]//读取值 
#define tmode saveData[4]//当前模式
#define tdw   saveData[5]//档位
#define sset  saveData[6]//读取值 
#define smode saveData[7]//当前模式
#define sdw   saveData[8]//档位

unsigned char setFlag  = 0;//设置标志

unsigned int i;
char dis0[128];//液晶显示暂存数组
char dis1[32];//液晶显示暂存数组

u8 needWriteFlash=0;//需要保存数据到flash

float nowTemp = 0;//当前温度
u16 sensorVal =0 ;//传感器值
u16 lightVal =0;//光照值
u16 adcx ; //ad采集值

u8 gasFanGrade = 0;//气体浓度对应的风扇需求档位，不写入Flash，不影响原tdw
u8 fanPwmOut = 0;//实际输出给风扇PWM的档位

u8 bjFlag =0;//异常提醒
u8 readFlag =0;//读取标志

u8 startCheckWireLess = 0;//启动检测无线种类
u8 wirelessFlag = 0;//无线种类 0蓝牙或无无线 1wifi

#define BUZ_PIN  PA5  //蜂鸣器接口

void ctrlLedPwm(u8 grade) //档位0-10
{
    My_PWM_SetDuty(TIM3,TIM_CH_2,grade*10);//设置pwm
}

void ctrlFanPwm(u8 grade) //档位0-10
{
    My_PWM_SetDuty(TIM1,TIM_CH_4,grade*10);//设置pwm
}

u8 getGasFanGrade(u16 gasVal,u16 gasSet)//根据气体超限程度计算排风需求档位
{
    u16 diff = 0;

    if(gasVal <= gasSet)
    {
        return 0;//气体未超限，不干预原风扇档位
    }

    diff = gasVal - gasSet;

    if(diff <= 5)
    {
        return 4;//轻微超限：至少4档排风
    }
    else if(diff <= 15)
    {
        return 7;//中等超限：至少7档排风
    }
    else
    {
        return 10;//严重超限：满速排风
    }
}

void limitSaveDataRange(void)//只限制容易导致功能失效或输出越界的关键参数
{
    if(sset < 1) sset = 1;
    if(sset > 99) sset = 99;//sensorVal最大约为99，sset超过99会导致气体报警永远不触发

    if(tdw < 0) tdw = 0;
    if(tdw > 10) tdw = 10;//风扇PWM档位限制在0-10

    if(smode != 0 && smode != 1) smode = 0;
    if(sdw < 0) sdw = 0;
    if(sdw > 1) sdw = 1;//排风状态只允许关闭0或启动1
}

const char *tel = "8615878488868";
unsigned int gsmReportDelay = 0;//gsm短信发送计时

int main(void)
{
    USARTx_Init(USART1,9600);//串口初始化为
    USARTx_Init(USART2,115200);//串口初始化
    My_LcdHmi_Init(USART3,115200);//串口初始化
    
    My_KEY_Init();//初始化输入
    My_LED_Init(); //输出初始化 

	My_ADC_Init(ADC1);//初始化adc  
    My_DS18B20_Init(PA4);//初始化DS18B20    
    
    My_PWM_Init(TIM1,TIM_CH_4,2000);//初始化定时器 通道 周期 该PWM无特殊要求        
    My_PWM_Init(TIM3,TIM_CH_2,2000);//初始化定时器 通道 周期 该PWM无特殊要求
    
    My_LEDBlink(BUZ_PIN,1,2,100,300);//控制蜂鸣器动作
    delay_ms(5);
    
    My_STMFlash_SectorSaveInit(ADDR_FLASH_WRITE,saveData,sizeof(saveData));//初始化flash
    limitSaveDataRange();//防止Flash中旧参数越界，尤其避免气体阈值超过99导致报警永远不触发
    
    buzzer = 1;ctrlPin = 1;ctrlLedPwm(5);ctrlFanPwm(5);//上电动作下
    delay_ms(100);
    buzzer = 0; ctrlPin = 0;ctrlLedPwm(0);ctrlFanPwm(0);//关闭动作        

    LCD_Init();   //tft初始化    
    LCD_Clear(Color16_BLACK);//清全屏    
    BACK_COLOR=Color16_BLACK;FRONT_COLOR=Color16_RED;	 //设置显示颜色  
    MyLCD_Show(6,0,"欢迎使用 ");//显示
    MyLCD_Show(4,1,"Init... ");//显示
    
	My_MLDC4G_Init();//gsm初始化 初始化中包括 哪个串口和GSM灯指示    
	
	
    i = 5;startCheckWireLess=1;//启动检测 一段时间取消
    while(i-- && wirelessFlag == 0) //此时判断是无线类型
    {USARTSendString(USART1,"ATE0\r\n");  delay_ms(100);}  
    startCheckWireLess = 0;//取消检测
    if(wirelessFlag == 1)
    {    
//        i = 50;while(i--){delay_ms(100);}      //wifi启动 必须延时5s或以上   
//    USARTSendString(USART1,"AT+CWMODE_CUR=3\r\n");// 设置模式
//    delay_ms(50); 
//    USARTSendString(USART1,"AT+CWSAP=\"AUAISOUT000\",\"\",1,0\r\n");//设置网络名字及密码
//    delay_ms(50);
//    USARTSendString(USART1,"AT+CIPAP_CUR=\"10.10.10.11\"\r\n");//设置本机ip
//    delay_ms(50);
//    USARTSendString(USART1,"AT+CIPMODE=0\r\n");//设置ip模式
//    delay_ms(50);
        USARTSendString(USART1,"AT+CIPMUX=1\r\n");//打开多连接
        delay_ms(50);
    }   
    
   while(1)
    {
        scanKeyAnddealKey();//按键扫描及处理
        if(myReadFlag_tick == true ) //定时读取时间到
        {
            myReadFlag_tick = false; //清除标志

            adcx=My_ADC_GetValue(ADC1,ADC_Channel_0);// ad读取
            lightVal =99-(float)adcx*99/4096;// 获取光照值	
            
            sensorVal =((float) My_ADC_GetValue(ADC1,ADC_Channel_1)*3.3/4096)*2*99.0/5.0;// 采集值  *2表示电阻分压	
			if(sensorVal<30)sensorVal = 0;//数据处理
			else if(sensorVal>75){sensorVal=99;}  //通过转换转化为0-99 其中参数根据实际测试获得的
			else
			{sensorVal = (sensorVal-30)*99/(75-30);}//转化                                    
            
            if(lmode == 0 )//自动模式
            {
                if(lightVal > lset-0*4)ldw = 0;//切换档位
                else if(lightVal > lset-1*4)ldw = 1;//切换档位
                else if(lightVal > lset-2*4)ldw = 2;//切换档位
                else if(lightVal > lset-3*4)ldw = 3;//切换档位
                else if(lightVal > lset-4*4)ldw = 4;//切换档位
                else if(lightVal > lset-5*4)ldw = 5;//切换档位
                else if(lightVal > lset-6*4)ldw = 6;//切换档位
                else if(lightVal > lset-7*4)ldw = 7;//切换档位
                else if(lightVal > lset-8*4)ldw = 8;//切换档位
                else if(lightVal > lset-9*4)ldw = 9;//切换档位
                else  ldw = 10;//切换档位
            }
            
            if(tmode == 0 )//自动模式
            {
				if(nowTemp<=tset+0*2){tdw = 0;}//pwm调整 0表示停止				
				else if(nowTemp<=tset+1*2) {tdw = 1;}//根据温度 调整pwm调整
				else if(nowTemp<=tset+2*2) {tdw = 2;}//根据温度 调整pwm调整
				else if(nowTemp<=tset+3*2) {tdw = 3;}//根据温度 调整pwm调整
				else if(nowTemp<=tset+4*2) {tdw = 4;}//根据温度 调整pwm调整
				else if(nowTemp<=tset+5*2) {tdw = 5;}//根据温度 调整pwm调整
				else if(nowTemp<=tset+6*2) {tdw = 6;}//根据温度 调整pwm调整
				else if(nowTemp<=tset+7*2) {tdw = 7;}//根据温度 调整pwm调整				
				else if(nowTemp<=tset+8*2) {tdw = 8;}//根据温度 调整pwm调整			
                else if(nowTemp<=tset+9*2) {tdw = 9;}//根据温度 调整pwm调整
				else {tdw = 10;}//根据温度 调整pwm调整   

                // 自动模式下tdw仍只由原温度逻辑计算。
                // 气体排风需求不直接改tdw，后面统一用fanPwmOut输出，避免影响APP、按键和Flash。
            }
            
            if(smode == 0 )//自动模式
            {
                if(sensorVal>sset)
                {
                    sdw  = 1;//打开
                }
            }

            if(sensorVal>sset)
            {
                bjFlag  = 1;My_LEDBlink(BUZ_PIN,1,2,50,2000);//控制蜂鸣器动作//报警
            }
            else{bjFlag = 0;}//停止报警 My_LEDBlink_Stop(BUZ_PIN);

            if(gsmReportDelay<500)gsmReportDelay++;//gsm发送延时
            if(gsmReportDelay > 150)//间隔时间超过
            {
                if(bjFlag != 0 )//依然处于报警状态
                {
                    gsmReportDelay = 0;//清零进入下一个周期   					
					sprintf(dis0,"Warning!GasHigh!!");//打印  	      
					My_MLDC4G_SendSMS(tel,dis0); //发送信息 					  					
                }				             
            }
            
            ctrlLedPwm(ldw);//pwm控制
            gasFanGrade = getGasFanGrade(sensorVal,sset);//根据气体浓度计算排风需求档位
            fanPwmOut = tdw;//先保留原来的风扇档位
            if(gasFanGrade > fanPwmOut)fanPwmOut = gasFanGrade;//气体超限时只提高实际输出，不修改tdw
            ctrlFanPwm(fanPwmOut);//pwm控制
            if(sdw == 1)
            {
                ctrlPin  = 1;//打开
            }
            else{ctrlPin = 0;}//关闭                   
            
            displayOfCollectedData();//显示采集到的数据    
            
            My_USART_printf(USART3,"main.vlight.txt=\"%d\"\xff\xff\xff",lightVal);
            My_USART_printf(USART3,"main.vsetl.txt=\"%d\"\xff\xff\xff",lset);
            My_USART_printf(USART3,"main.lmode.txt=\"%d\"\xff\xff\xff",lmode);
            My_USART_printf(USART3,"main.ldw.txt=\"%d\"\xff\xff\xff",ldw);
            
            My_USART_printf(USART3,"main.vtemp.txt=\"%4.1f\"\xff\xff\xff",nowTemp);
            My_USART_printf(USART3,"main.vsett.txt=\"%d\"\xff\xff\xff",tset);
            My_USART_printf(USART3,"main.tmode.txt=\"%d\"\xff\xff\xff",tmode);
            My_USART_printf(USART3,"main.tdw.txt=\"%d\"\xff\xff\xff",fanPwmOut);
            
            My_USART_printf(USART3,"main.vcjz.txt=\"气体浓度:%d\"\xff\xff\xff",sensorVal);            
            My_USART_printf(USART3,"main.vsetc.txt=\"%d\"\xff\xff\xff",sset);   
            My_USART_printf(USART3,"main.smode.txt=\"%d\"\xff\xff\xff",smode);
            My_USART_printf(USART3,"main.sdw.txt=\"%d\"\xff\xff\xff",sdw);
            
            if(needWriteFlash > 0)
            {
                if(needWriteFlash > 1) needWriteFlash--;//延时保存，避免连续按键或APP连续设置时频繁写Flash

                if(needWriteFlash == 1)
                {
                    limitSaveDataRange();//保存前再次限制范围，防止错误参数写入Flash
                    My_STMFlash_SaveUseSector(saveData);   //将数据保存到flash 掉电不丢失      
                    needWriteFlash =0;//0 直接停止更新 除非下次触发
                }
            }              
            
        }
                
        if(mySendFlag_tick == true )//定时发送时间到
        {
            mySendFlag_tick = false;//清除标志
            
            readFlag++;//读取参数
            if(readFlag>=2)//读取完成
            {
                readFlag =0;//清空标志
                nowTemp = My_DS18B20_GetTemp(PA4);	//获取温度        
            }
            
            sprintf(dis0,"*Ng%02dSg%02dMg%dDg%02d",lightVal,lset,lmode,ldw);//打印            
            sprintf(dis0,"%sNt%04.1fSt%02dMt%dDt%02d",dis0,nowTemp,tset,tmode,fanPwmOut);//打印，Dt字段保留原格式，数值改为实际PWM输出档位
            sprintf(dis0,"%sNs气体浓度:%02dSs%02dMs%dDs%02d#",dis0,sensorVal,sset,smode,sdw);//打印                                     
            if(wirelessFlag == 0)USARTSendString(USART1 ,dis0);	 //发送	 对应的 数据
        }
        My_UartMessage_Process();//处理串口数据      
        My_LcdHmiMessage_Process();        
    }
}



void scanKeyAnddealKey(void)
{
    My_KeyScan();//按键扫描
    
    if(KeyIsPress(KEY_1))
    {
        setFlag++;//设置标志
        if(setFlag == 1) setFlag = 2;//TFT不显示光照设置项，本地按键跳过，保留APP/底层协议不变
        if(setFlag == 4) setFlag = 5;//TFT不显示灯光设置项，本地按键跳过，保留APP/底层协议不变
        if(setFlag >= 7) setFlag = 0;     
        My_LEDBlink(BUZ_PIN,1,1,50,100);//控制蜂鸣器动作//报警                       
    }
    if(KeyIsPress(KEY_2))//按键按下
    {
        if(setFlag==1)      //处于设置
        {
            if(lset > 0 ){lset=lset-1;}
        }
        else if(setFlag==2)//处于设置
        {
            if(tset > 0){tset=tset-1;}
        } 
        else if(setFlag==3)//处于设置2
        {
            if(sset > 1){sset=sset-1;}
        }    
        else if(setFlag==4)//处于设置
        {
            if(lmode != 0 ) {if(ldw>0)ldw=ldw-1;}//设置
        } 
        else if(setFlag==5)//处于设置
        {
            if(tmode != 0 )  {if(tdw>0)tdw=tdw-1;}//设置
        } 
        else if(setFlag==6)//处于设置
        {
            if(smode != 0 )  {sdw=0;}//手动模式下关闭排风
        }         
        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);  //将数据保存到flash 掉电不丢失       
        My_LEDBlink(BUZ_PIN,1,1,50,100);//控制蜂鸣器动作//报警               
    }
    if(KeyIsPress(KEY_3))//按键按下
    {
        if(setFlag==1)      //处于设置
        {
            if(lset < 999 ){lset=lset+1;}
        }
        else if(setFlag==2)//处于设置
        {
            if(tset < 999 ) {tset=tset+1;}
        }       
        else if(setFlag==3)//处于设置
        {
            if(sset < 99 ) {sset=sset+1;}
        } 
        else if(setFlag==4)//处于设置
        {
            if(lmode != 0 ) {if(ldw<10)ldw=ldw+1;}//设置
        } 
        else if(setFlag==5)//处于设置
        {
            if(tmode != 0 ) {if(tdw<10)tdw=tdw+1;}//设置
        } 
        else if(setFlag==6)//处于设置
        {
            if(smode != 0 )  {sdw=1;}//手动模式下启动排风
        } 

        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);   //将数据保存到flash 掉电不丢失  
        My_LEDBlink(BUZ_PIN,1,1,50,100);//控制蜂鸣器动作//报警                       
    }
    if(KeyIsPress(KEY_4))
    {
        if(setFlag==4)//处于设置
        {
            if(lmode != 0 ) {lmode=0;}//切换模式
            else lmode = 1;//切换模式
        }   
        if(setFlag==5)//处于设置
        {
            if(tmode != 0 ) {tmode=0;}//切换模式
            else tmode = 1;//切换模式
        }    
        if(setFlag==6)//处于设置
        {
            if(smode != 0 ) {smode=0;}//切换模式
            else smode = 1;//切换模式
        }    

        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);   //将数据保存到flash 掉电不丢失 
        My_LEDBlink(BUZ_PIN,1,1,50,100);//控制蜂鸣器动作//报警                       
    } 

    limitSaveDataRange();//本地按键处理后统一限制参数范围，避免阈值或档位越界
}

void displayOfCollectedData(void)
{
    static u16 remupData_01 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间
    static float remupData_02 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间
    static u16 remupData_03 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间
    static u16 remupData_04 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间
    static u16 remupData_05 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间
    static u16 remupData_06 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间
    static u16 remupData_07 = 0xff;//记录数据变量 用于查看对应数据是否变化,变化了更新显示，避免反复更新显示占用时间    
    
    yPlace = 1;//显示行位置

    /* 仅取消TFT界面上的光照显示：
       不修改光照采集、不修改APP上传协议、不修改WiFi/GSM原有通信流程。
       因为这里不再执行 yPlace++，所以下面的温度会显示到原来“光照”的位置。 */

    if(remupData_02 != nowTemp + tset )//读取数据发生了变化
    {
        remupData_02 = nowTemp + tset;//记录本次数据
        if(nowTemp>tset)FRONT_COLOR = Color16_RED;	 //设置显示颜色 
        else FRONT_COLOR = Color16_WHITE;	 //设置显示颜色 
        sprintf(dis0,"温  度:%4.1f'C s%02d ",nowTemp , tset);//打印                
        MyLCD_Show(0,yPlace++,dis0);//显示
    }else{yPlace++;}         

    if(remupData_03 != sensorVal + sset )//读取数据发生了变化
    {
        remupData_03 = sensorVal + sset;//Color16_RED
        if(sensorVal>sset)FRONT_COLOR = Color16_RED;	 //设置显示颜色 
        else FRONT_COLOR = Color16_WHITE;	 //设置显示颜色 
        sprintf(dis0,"气体浓度:%02d   s%02d ",sensorVal , sset);//打印                
        MyLCD_Show(0,yPlace++,dis0);//显示
    }else{yPlace++;}

    /* 取消TFT界面上的“灯光”档位显示：
       不修改灯光相关变量、APP协议和原有控制逻辑，只是不在TFT屏幕显示。
       因为这里不再执行 yPlace++，所以下面的风扇、设备、异常提醒会整体上移一行。 */

    if(remupData_05 != (fanPwmOut*10 + tmode))//读取数据发生了变化
    {
        remupData_05 = (fanPwmOut*10 + tmode);//记录本次数据
        FRONT_COLOR = Color16_YELLOW;	 //设置显示颜色        
        sprintf(dis0,"风扇:%02d档    %s ",fanPwmOut,tmode==0?"自动":"手动");//打印
        MyLCD_Show(0,yPlace++,dis0);//显示   
    }else{yPlace++;}

    if(remupData_06 != (sdw*10 + smode))//读取数据发生了变化，避免sdw+smode不同状态相加后数值相同导致不刷新
    {
        remupData_06 = (sdw*10 + smode);//记录本次数据
        if(sdw == 1)
        {
            FRONT_COLOR = Color16_YELLOW;	 //设置显示颜色                
            sprintf(dis0,"排风:启动    %s ",smode==0?"自动":"手动");//打印            
        }
        else
        {
            FRONT_COLOR = Color16_WHITE;	 //设置显示颜色
            sprintf(dis0,"排风:关闭    %s ",smode==0?"自动":"手动");//打印
        }

        MyLCD_Show(0,yPlace++,dis0);//显示        
    }else{yPlace++;}         

    if(remupData_07 != bjFlag)//读取数据发生了变化
    {
        remupData_07 = bjFlag;//记录本次数据
        if(bjFlag != 0)
        {
            FRONT_COLOR = Color16_RED;	 //气体报警触发时用红色提示
            MyLCD_Show(0,yPlace++,"气体报警:触发");//显示               
        }
        else
        {
            FRONT_COLOR = Color16_WHITE;	 //正常状态用白色显示
            MyLCD_Show(0,yPlace++,"气体报警:正常");//显示                   
        }     
    }else{yPlace++;}  
    
    FRONT_COLOR = Color16_RED;	 //设置显示颜色 

    /* 光照和灯光显示行已取消，右侧“<<”指示重新对应当前显示行。
       本地按键已跳过setFlag=1光照项和setFlag=4灯光项，但APP/底层协议仍保留。 */
    for(i=1;i<=6;i++)//先清除右侧设置标记，避免残留
    {
        MyLCD_Show(18,i,"  ");
    }
    if(setFlag == 2) MyLCD_Show(18,1,"<<");//温度阈值
    if(setFlag == 3) MyLCD_Show(18,2,"<<");//气体阈值
    if(setFlag == 5) MyLCD_Show(18,3,"<<");//风扇档位/模式
    if(setFlag == 6) MyLCD_Show(18,4,"<<");//排风状态/模式
}


void OnGetLcdHmiMessage(const _lcdHmi_msg_obj *lcdHmiMsgRec)
{
    char *strPtr;
    if((strPtr=strstr(lcdHmiMsgRec->payload,"SL"))!=NULL)//接收到字符串 
    {
        lset=ParseInteger(strPtr+2,2);//提取设置参数        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"ST"))!=NULL)//接收到字符串
    {
        tset=ParseInteger(strPtr+2,2);//提取设置参数        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"SC"))!=NULL)//接收到字符串 
    {
        sset=ParseInteger(strPtr+2,2);//提取设置参数
        limitSaveDataRange();//限制气体阈值最大99，避免报警条件失效        
    } 

    if((strPtr=strstr(lcdHmiMsgRec->payload,"ML"))!=NULL)//接收到字符串 
    {
        lmode=ParseInteger(strPtr+2,1);//提取设置参数        
    } 
    if((strPtr=strstr(lcdHmiMsgRec->payload,"MT"))!=NULL)//接收到字符串 
    {
        tmode=ParseInteger(strPtr+2,1);//提取设置参数        
    } 
    if((strPtr=strstr(lcdHmiMsgRec->payload,"MS"))!=NULL)//接收到字符串 
    {
        smode=ParseInteger(strPtr+2,1);//提取设置参数        
    } 
    
    if((strPtr=strstr(lcdHmiMsgRec->payload,"LD"))!=NULL)//接收到字符串 
    {
        if(lmode==1)ldw=ParseInteger(strPtr+2,2);//提取设置参数        
    }       
    if((strPtr=strstr(lcdHmiMsgRec->payload,"TD"))!=NULL)//接收到字符串 
    {
        if(tmode==1)tdw=ParseInteger(strPtr+2,2);//提取设置参数        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"SD"))!=NULL)//接收到字符串 
    {
        if(smode==1)sdw=ParseInteger(strPtr+2,2);//提取设置参数        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"HMI"))!=NULL)//接收到字符串     
    {
    }

    limitSaveDataRange();//HMI命令处理完成后统一限制参数范围
               
}



void OnGetUartMessage(const _uart_msg_obj *uartMsgRec)
{
    char *strPtr;
    u8 buzFlag =0;//是否需要蜂鸣器提醒
    if((strPtr=strstr(uartMsgRec->payload,"SL"))!=NULL)//接收到字符串 
    {
        lset=ParseInteger(strPtr+2,2);//提取设置参数       
        buzFlag =1;  //需要提醒      
    }
    if((strPtr=strstr(uartMsgRec->payload,"ST"))!=NULL)//接收到字符串
    {
        tset=ParseInteger(strPtr+2,2);//提取设置参数  
        buzFlag =1;  //需要提醒              
    }
    if((strPtr=strstr(uartMsgRec->payload,"SC"))!=NULL)//接收到字符串 
    {
        sset=ParseInteger(strPtr+2,2);//提取设置参数
        limitSaveDataRange();//限制气体阈值最大99，避免报警条件失效        
        buzFlag =1;  //需要提醒      
    } 

    if((strPtr=strstr(uartMsgRec->payload,"ML"))!=NULL)//接收到字符串 
    {
        lmode=ParseInteger(strPtr+2,1);//提取设置参数        
        buzFlag =1;  //需要提醒      
    } 
    if((strPtr=strstr(uartMsgRec->payload,"MT"))!=NULL)//接收到字符串 
    {
        tmode=ParseInteger(strPtr+2,1);//提取设置参数        
        buzFlag =1;  //需要提醒      
    } 
    if((strPtr=strstr(uartMsgRec->payload,"MS"))!=NULL)//接收到字符串 
    {
        smode=ParseInteger(strPtr+2,1);//提取设置参数        
        buzFlag =1;  //需要提醒      
    } 
    
    if((strPtr=strstr(uartMsgRec->payload,"LD"))!=NULL)//接收到字符串 
    {
        if(lmode==1)ldw=ParseInteger(strPtr+2,2);//提取设置参数    
        buzFlag =1;  //需要提醒              
    }       
    if((strPtr=strstr(uartMsgRec->payload,"TD"))!=NULL)//接收到字符串 
    {
        if(tmode==1)tdw=ParseInteger(strPtr+2,2);//提取设置参数        
        buzFlag =1;  //需要提醒      
    }
    if((strPtr=strstr(uartMsgRec->payload,"SD"))!=NULL)//接收到字符串 
    {
        if(smode==1)sdw=ParseInteger(strPtr+2,2);//提取设置参数        
        buzFlag =1;  //需要提醒      
    }

    if((strPtr=strstr(uartMsgRec->payload,"%"))!=NULL)//接收到字符串
    {
        if(wirelessFlag == 0)USARTSendString(USART1,"\r\nTest Ok!!\r\n\r\n");	 //发送	 对应的 数据
    }        
    
    limitSaveDataRange();//APP命令处理完成后统一限制参数范围

    if(buzFlag == 1)//需要提醒
    {
        buzFlag = 0;//清空标志
        My_LEDBlink(BUZ_PIN,1,1,50,100);//控制蜂鸣器动作//报警       
        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);   //将数据保存到flash 掉电不丢失                    
    }  
}


void My_ESP8266_SendStrStr(USART_TypeDef* USARTx, const char *str)
{
//    u8 i;
//    for(i=0;i<2;i++)
    {
        My_USART_printf(USARTx,"AT+CIPSEND=%d,%d\r\n",0,strlen(str));  
        delay_ms(10);
    }
}

void checkWireLessMode(u8 recBuf)//检查无线类型
{
    static u8 checkIn = 0;//查看第一次标志是否接受
    if(startCheckWireLess == 1 && wirelessFlag == 0)//启动检测无线
    {
        if(recBuf == 'O')checkIn = 1;
 
    }
}
