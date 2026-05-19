#include "my_include.h"

#define F_SIZE      12 //ʾС ʾҪģſ
#define MyLCD_Show(m,n,p)     LCD_ShowString(LCD_GetPos_X(F_SIZE,m),LCD_GetPos_Y(16,n),p,F_SIZE,false)  //ʾ 
u16 yPlace = 0; //ʾλyֵ

void scanKeyAnddealKey(void);
void My_ESP8266_SendStrStr(USART_TypeDef* USARTx, const char *str);
unsigned char makeSureLinkCount=0; //ȷӱ
void displayOfCollectedData(void);//ʾɼ

#define ADDR_FLASH_WRITE            (FLASH_BASE_ADDR+STM32_FLASH_SIZE*1024-STM_SECTOR_SIZE*1)
int16 saveData[10] = {60,0,3,32,0,2,10,0,0,10};

#define lset  saveData[0]//ֵ
#define lmode saveData[1]//ǰģʽ
#define ldw   saveData[2]//λ
#define tset  saveData[3]//ȡֵ 
#define tmode saveData[4]//ǰģʽ
#define tdw   saveData[5]//λ
#define sset  saveData[6]//ȡֵ 
#define smode saveData[7]//ǰģʽ
#define sdw   saveData[8]//λ

unsigned char setFlag  = 0;//ñ־

unsigned int i;
char dis0[128];//Һʾݴ
char dis1[32];//Һʾݴ

u8 needWriteFlash=0;//Ҫݵflash

float nowTemp = 0;//ǰ¶
u16 sensorVal =0 ;//ֵ
u16 lightVal =0;//ֵ
u16 adcx ; //adɼֵ

u8 gasFanGrade = 0;//ŨȶӦķλдFlashӰԭtdw
u8 fanPwmOut = 0;//ʵPWMĵλ

u8 bjFlag =0;//쳣
u8 readFlag =0;//ȡ־

u8 startCheckWireLess = 0;//
u8 wirelessFlag = 0;// 0 1wifi

#define BUZ_PIN  PA5  //ӿ

void ctrlLedPwm(u8 grade) //λ0-10
{
    My_PWM_SetDuty(TIM3,TIM_CH_2,grade*10);//pwm
}

void ctrlFanPwm(u8 grade) //λ0-10
{
    My_PWM_SetDuty(TIM1,TIM_CH_4,grade*10);//pwm
}

u8 getGasFanGrade(u16 gasVal,u16 gasSet)//峬޳̶ȼŷλ
{
    u16 diff = 0;

    if(gasVal <= gasSet)
    {
        return 0;//δޣԤԭȵλ
    }

    diff = gasVal - gasSet;

    if(diff <= 5)
    {
        return 4;//΢ޣ4ŷ
    }
    else if(diff <= 15)
    {
        return 7;//еȳޣ7ŷ
    }
    else
    {
        return 10;//سޣŷ
    }
}

void limitSaveDataRange(void)//ֻ׵¹ʧЧԽĹؼ
{
    if(sset < 1) sset = 1;
    if(sset > 99) sset = 99;//sensorValԼΪ99sset99ᵼ屨Զ

    if(tdw < 0) tdw = 0;
    if(tdw > 10) tdw = 10;//PWMλ0-10

    if(smode != 0 && smode != 1) smode = 0;
    if(sdw < 0) sdw = 0;
    if(sdw > 1) sdw = 1;//ŷ״ֻ̬ر01
}

const char *tel = "8615878488868";
unsigned int gsmReportDelay = 0;//gsmŷͼʱ

int main(void)
{
    USARTx_Init(USART1,9600);//ڳʼΪ
    USARTx_Init(USART2,115200);//ڳʼ
    My_LcdHmi_Init(USART3,115200);//ڳʼ
    
    My_KEY_Init();//ʼ
    My_LED_Init(); //ʼ 

	My_ADC_Init(ADC1);//ʼadc  
    My_DS18B20_Init(PA4);//ʼDS18B20    
    
    My_PWM_Init(TIM1,TIM_CH_4,2000);//ʼʱ ͨ  PWMҪ        
    My_PWM_Init(TIM3,TIM_CH_2,2000);//ʼʱ ͨ  PWMҪ
    
    My_LEDBlink(BUZ_PIN,1,2,100,300);//Ʒ
    delay_ms(5);
    
    My_STMFlash_SectorSaveInit(ADDR_FLASH_WRITE,saveData,sizeof(saveData));//ʼflash
    limitSaveDataRange();//ֹFlashоɲԽ磬ֵ99±Զ
    
    buzzer = 1;ctrlPin = 1;ctrlLedPwm(5);ctrlFanPwm(5);//ϵ綯
    delay_ms(100);
    buzzer = 0; ctrlPin = 0;ctrlLedPwm(0);ctrlFanPwm(0);//رն        

    LCD_Init();   //tftʼ    
    LCD_Clear(Color16_BLACK);//ȫ    
    BACK_COLOR=Color16_BLACK;FRONT_COLOR=Color16_RED;	 //ʾɫ  
    MyLCD_Show(6,0,"欢迎使用");//显示
    MyLCD_Show(4,1,"Init... ");//ʾ
    
	My_MLDC4G_Init();//gsmʼ ʼа ĸںGSMָʾ    
	
	
    i = 5;startCheckWireLess=1;// һʱȡ
    while(i-- && wirelessFlag == 0) //ʱж
    {USARTSendString(USART1,"ATE0\r\n");  delay_ms(100);}  
    startCheckWireLess = 0;//ȡ
    if(wirelessFlag == 1)
    {    
//        i = 50;while(i--){delay_ms(100);}      //wifi ʱ5s   
//    USARTSendString(USART1,"AT+CWMODE_CUR=3\r\n");// ģʽ
//    delay_ms(50); 
//    USARTSendString(USART1,"AT+CWSAP=\"AUAISOUT000\",\"\",1,0\r\n");//ּ
//    delay_ms(50);
//    USARTSendString(USART1,"AT+CIPAP_CUR=\"10.10.10.11\"\r\n");//ñip
//    delay_ms(50);
//    USARTSendString(USART1,"AT+CIPMODE=0\r\n");//ipģʽ
//    delay_ms(50);
        USARTSendString(USART1,"AT+CIPMUX=1\r\n");//򿪶
        delay_ms(50);
    }   
    
   while(1)
    {
        scanKeyAnddealKey();//ɨ輰
        if(myReadFlag_tick == true ) //ʱȡʱ䵽
        {
            myReadFlag_tick = false; //־

            adcx=My_ADC_GetValue(ADC1,ADC_Channel_0);// adȡ
            lightVal =99-(float)adcx*99/4096;// ȡֵ	
            
            sensorVal =((float) My_ADC_GetValue(ADC1,ADC_Channel_1)*3.3/4096)*2*99.0/5.0;// ɼֵ  *2ʾѹ	
			if(sensorVal<30)sensorVal = 0;//ݴ
			else if(sensorVal>75){sensorVal=99;}  //ͨתתΪ0-99 вʵʲԻõ
			else
			{sensorVal = (sensorVal-30)*99/(75-30);}//ת                                    
            
            if(lmode == 0 )//Զģʽ
            {
                if(lightVal > lset-0*4)ldw = 0;//лλ
                else if(lightVal > lset-1*4)ldw = 1;//лλ
                else if(lightVal > lset-2*4)ldw = 2;//лλ
                else if(lightVal > lset-3*4)ldw = 3;//лλ
                else if(lightVal > lset-4*4)ldw = 4;//лλ
                else if(lightVal > lset-5*4)ldw = 5;//лλ
                else if(lightVal > lset-6*4)ldw = 6;//лλ
                else if(lightVal > lset-7*4)ldw = 7;//лλ
                else if(lightVal > lset-8*4)ldw = 8;//лλ
                else if(lightVal > lset-9*4)ldw = 9;//лλ
                else  ldw = 10;//лλ
            }
            
            if(tmode == 0 )//Զģʽ
            {
				if(nowTemp<=tset+0*2){tdw = 0;}//pwm 0ʾֹͣ				
				else if(nowTemp<=tset+1*2) {tdw = 1;}//¶ pwm
				else if(nowTemp<=tset+2*2) {tdw = 2;}//¶ pwm
				else if(nowTemp<=tset+3*2) {tdw = 3;}//¶ pwm
				else if(nowTemp<=tset+4*2) {tdw = 4;}//¶ pwm
				else if(nowTemp<=tset+5*2) {tdw = 5;}//¶ pwm
				else if(nowTemp<=tset+6*2) {tdw = 6;}//¶ pwm
				else if(nowTemp<=tset+7*2) {tdw = 7;}//¶ pwm				
				else if(nowTemp<=tset+8*2) {tdw = 8;}//¶ pwm			
                else if(nowTemp<=tset+9*2) {tdw = 9;}//¶ pwm
				else {tdw = 10;}//¶ pwm   

                // Զģʽtdwֻԭ¶߼㡣
                // ŷֱӸtdwͳһfanPwmOutӰAPPFlash
            }
            
            if(smode == 0 )//Զģʽ
            {
                if(sensorVal>sset)
                {
                    sdw  = 1;//
                }
            }

            if(sensorVal>sset)
            {
                bjFlag  = 1;My_LEDBlink(BUZ_PIN,1,2,50,2000);//Ʒ//
            }
            else{bjFlag = 0;}//ֹͣ My_LEDBlink_Stop(BUZ_PIN);

            if(gsmReportDelay<500)gsmReportDelay++;//gsmʱ
            if(gsmReportDelay > 150)//ʱ䳬
            {
                if(bjFlag != 0 )//Ȼڱ״̬
                {
                    gsmReportDelay = 0;//һ   					
					sprintf(dis0,"Warning!GasHigh!!");//ӡ  	      
					My_MLDC4G_SendSMS(tel,dis0); //Ϣ 					  					
                }				             
            }
            
            ctrlLedPwm(ldw);//pwm
            gasFanGrade = getGasFanGrade(sensorVal,sset);//Ũȼŷλ
            fanPwmOut = tdw;//ȱԭķȵλ
            if(gasFanGrade > fanPwmOut)fanPwmOut = gasFanGrade;//峬ʱֻʵ޸tdw
            ctrlFanPwm(fanPwmOut);//pwm
            if(sdw == 1)
            {
                ctrlPin  = 1;//
            }
            else{ctrlPin = 0;}//ر                   
            
            displayOfCollectedData();//ʾɼ    
            
            My_USART_printf(USART3,"main.vlight.txt=\"%d\"\xff\xff\xff",lightVal);
            My_USART_printf(USART3,"main.vsetl.txt=\"%d\"\xff\xff\xff",lset);
            My_USART_printf(USART3,"main.lmode.txt=\"%d\"\xff\xff\xff",lmode);
            My_USART_printf(USART3,"main.ldw.txt=\"%d\"\xff\xff\xff",ldw);
            
            My_USART_printf(USART3,"main.vtemp.txt=\"%4.1f\"\xff\xff\xff",nowTemp);
            My_USART_printf(USART3,"main.vsett.txt=\"%d\"\xff\xff\xff",tset);
            My_USART_printf(USART3,"main.tmode.txt=\"%d\"\xff\xff\xff",tmode);
            My_USART_printf(USART3,"main.tdw.txt=\"%d\"\xff\xff\xff",fanPwmOut);
            
            My_USART_printf(USART3,"main.vcjz.txt=\"Ũ:%d\"\xff\xff\xff",sensorVal);            
            My_USART_printf(USART3,"main.vsetc.txt=\"%d\"\xff\xff\xff",sset);   
            My_USART_printf(USART3,"main.smode.txt=\"%d\"\xff\xff\xff",smode);
            My_USART_printf(USART3,"main.sdw.txt=\"%d\"\xff\xff\xff",sdw);
            
            if(needWriteFlash > 0)
            {
                if(needWriteFlash > 1) needWriteFlash--;//ʱ棬APPʱƵдFlash

                if(needWriteFlash == 1)
                {
                    limitSaveDataRange();//ǰٴƷΧֹдFlash
                    My_STMFlash_SaveUseSector(saveData);   //ݱ浽flash 粻ʧ      
                    needWriteFlash =0;//0 ֱֹͣ ´δ
                }
            }              
            
        }
                
        if(mySendFlag_tick == true )//ʱʱ䵽
        {
            mySendFlag_tick = false;//־
            
            readFlag++;//ȡ
            if(readFlag>=2)//ȡ
            {
                readFlag =0;//ձ־
                nowTemp = My_DS18B20_GetTemp(PA4);	//ȡ¶        
            }
            
            sprintf(dis0,"*Ng%02dSg%02dMg%dDg%02d",lightVal,lset,lmode,ldw);//ӡ            
            sprintf(dis0,"%sNt%04.1fSt%02dMt%dDt%02d",dis0,nowTemp,tset,tmode,fanPwmOut);//ӡDtֶαԭʽֵΪʵPWMλ
            sprintf(dis0,"%sNsŨ:%02dSs%02dMs%dDs%02d#",dis0,sensorVal,sset,smode,sdw);//ӡ                                     
            if(wirelessFlag == 0)USARTSendString(USART1 ,dis0);	 //	 Ӧ 
        }
        My_UartMessage_Process();//      
        My_LcdHmiMessage_Process();        
    }
}



void scanKeyAnddealKey(void)
{
    My_KeyScan();//ɨ
    
    if(KeyIsPress(KEY_1))
    {
        setFlag++;//ñ־
        if(setFlag == 1) setFlag = 2;//TFTʾذAPP/ײЭ鲻
        if(setFlag == 4) setFlag = 5;//TFTʾƹذAPP/ײЭ鲻
        if(setFlag >= 7) setFlag = 0;     
        My_LEDBlink(BUZ_PIN,1,1,50,100);//Ʒ//                       
    }
    if(KeyIsPress(KEY_2))//
    {
        if(setFlag==1)      //
        {
            if(lset > 0 ){lset=lset-1;}
        }
        else if(setFlag==2)//
        {
            if(tset > 0){tset=tset-1;}
        } 
        else if(setFlag==3)//2
        {
            if(sset > 1){sset=sset-1;}
        }    
        else if(setFlag==4)//
        {
            if(lmode != 0 ) {if(ldw>0)ldw=ldw-1;}//
        } 
        else if(setFlag==5)//
        {
            if(tmode != 0 )  {if(tdw>0)tdw=tdw-1;}//
        } 
        else if(setFlag==6)//
        {
            if(smode != 0 )  {sdw=0;}//ֶģʽ¹رŷ
        }         
        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);  //ݱ浽flash 粻ʧ       
        My_LEDBlink(BUZ_PIN,1,1,50,100);//Ʒ//               
    }
    if(KeyIsPress(KEY_3))//
    {
        if(setFlag==1)      //
        {
            if(lset < 999 ){lset=lset+1;}
        }
        else if(setFlag==2)//
        {
            if(tset < 999 ) {tset=tset+1;}
        }       
        else if(setFlag==3)//
        {
            if(sset < 99 ) {sset=sset+1;}
        } 
        else if(setFlag==4)//
        {
            if(lmode != 0 ) {if(ldw<10)ldw=ldw+1;}//
        } 
        else if(setFlag==5)//
        {
            if(tmode != 0 ) {if(tdw<10)tdw=tdw+1;}//
        } 
        else if(setFlag==6)//
        {
            if(smode != 0 )  {sdw=1;}//ֶģʽŷ
        } 

        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);   //ݱ浽flash 粻ʧ  
        My_LEDBlink(BUZ_PIN,1,1,50,100);//Ʒ//                       
    }
    if(KeyIsPress(KEY_4))
    {
        if(setFlag==4)//
        {
            if(lmode != 0 ) {lmode=0;}//лģʽ
            else lmode = 1;//лģʽ
        }   
        if(setFlag==5)//
        {
            if(tmode != 0 ) {tmode=0;}//лģʽ
            else tmode = 1;//лģʽ
        }    
        if(setFlag==6)//
        {
            if(smode != 0 ) {smode=0;}//лģʽ
            else smode = 1;//лģʽ
        }    

        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);   //ݱ浽flash 粻ʧ 
        My_LEDBlink(BUZ_PIN,1,1,50,100);//Ʒ//                       
    } 

    limitSaveDataRange();//ذͳһƲΧֵλԽ
}

void displayOfCollectedData(void)
{
    static u16 remupData_01 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ
    static float remupData_02 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ
    static u16 remupData_03 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ
    static u16 remupData_04 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ
    static u16 remupData_05 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ
    static u16 remupData_06 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ
    static u16 remupData_07 = 0xff;//¼ݱ ڲ鿴ӦǷ仯,仯˸ʾⷴʾռʱ    
    
    yPlace = 1;//ʾλ

    /* ȡTFTϵĹʾ
       ޸Ĺղɼ޸APPϴЭ顢޸WiFi/GSMԭ̡ͨ
       Ϊﲻִ yPlace++¶Ȼʾԭաλá */

    if(remupData_02 != nowTemp + tset )//ȡݷ˱仯
    {
        remupData_02 = nowTemp + tset;//¼
        if(nowTemp>tset)FRONT_COLOR = Color16_RED;	 //ʾɫ 
        else FRONT_COLOR = Color16_WHITE;	 //ʾɫ 
        sprintf(dis0,"温度:%4.1fC s%02d",nowTemp , tset);//显示                
        MyLCD_Show(0,yPlace++,dis0);//ʾ
    }else{yPlace++;}         

    if(remupData_03 != sensorVal + sset )//ȡݷ˱仯
    {
        remupData_03 = sensorVal + sset;//Color16_RED
        if(sensorVal>sset)FRONT_COLOR = Color16_RED;	 //ʾɫ 
        else FRONT_COLOR = Color16_WHITE;	 //ʾɫ 
        sprintf(dis0,"气体浓度:%02d s%02d",sensorVal , sset);//显示                
        MyLCD_Show(0,yPlace++,dis0);//ʾ
    }else{yPlace++;}

    /* ȡTFTϵġƹ⡱λʾ
       ޸ĵƹرAPPЭԭп߼ֻǲTFTĻʾ
       Ϊﲻִ yPlace++ķȡ豸쳣ѻһС */

    if(remupData_05 != (fanPwmOut*10 + tmode))//ȡݷ˱仯
    {
        remupData_05 = (fanPwmOut*10 + tmode);//¼
        FRONT_COLOR = Color16_YELLOW;	 //ʾɫ        
        sprintf(dis0,"风扇:%02d档 %s",fanPwmOut,tmode==0?"自动":"手动");//显示
        MyLCD_Show(0,yPlace++,dis0);//ʾ   
    }else{yPlace++;}

    if(remupData_06 != (sdw*10 + smode))//ȡݷ˱仯sdw+smodeͬ״̬Ӻֵͬ²ˢ
    {
        remupData_06 = (sdw*10 + smode);//¼
        if(sdw == 1)
        {
            FRONT_COLOR = Color16_YELLOW;	 //ʾɫ                
            sprintf(dis0,"排风:启动 %s",smode==0?"自动":"手动");//显示            
        }
        else
        {
            FRONT_COLOR = Color16_WHITE;	 //ʾɫ
            sprintf(dis0,"排风:关闭 %s",smode==0?"自动":"手动");//显示
        }

        MyLCD_Show(0,yPlace++,dis0);//ʾ        
    }else{yPlace++;}         

    if(remupData_07 != bjFlag)//ȡݷ˱仯
    {
        remupData_07 = bjFlag;//¼
        if(bjFlag != 0)
        {
            FRONT_COLOR = Color16_RED;	 //屨ʱúɫʾ
            MyLCD_Show(0,yPlace++,"异常提醒:启动");//显示               
        }
        else
        {
            FRONT_COLOR = Color16_WHITE;	 //״̬ðɫʾ
            MyLCD_Show(0,yPlace++,"异常提醒:关闭");//显示                   
        }     
    }else{yPlace++;}  
    
    FRONT_COLOR = Color16_RED;	 //ʾɫ 

    /* պ͵ƹʾȡҲࡰ<<ָʾ¶ӦǰʾС
       ذsetFlag=1setFlag=4ƹAPP/ײЭԱ */
    for(i=1;i<=6;i++)//Ҳñǣ
    {
        MyLCD_Show(18,i,"  ");
    }
    if(setFlag == 2) MyLCD_Show(18,1,"<<");//¶ֵ
    if(setFlag == 3) MyLCD_Show(18,2,"<<");//ֵ
    if(setFlag == 5) MyLCD_Show(18,3,"<<");//ȵλ/ģʽ
    if(setFlag == 6) MyLCD_Show(18,4,"<<");//ŷ״̬/ģʽ
}


void OnGetLcdHmiMessage(const _lcdHmi_msg_obj *lcdHmiMsgRec)
{
    char *strPtr;
    if((strPtr=strstr(lcdHmiMsgRec->payload,"SL"))!=NULL)//յַ 
    {
        lset=ParseInteger(strPtr+2,2);//ȡò        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"ST"))!=NULL)//յַ
    {
        tset=ParseInteger(strPtr+2,2);//ȡò        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"SC"))!=NULL)//յַ 
    {
        sset=ParseInteger(strPtr+2,2);//ȡò
        limitSaveDataRange();//ֵ99ⱨʧЧ        
    } 

    if((strPtr=strstr(lcdHmiMsgRec->payload,"ML"))!=NULL)//յַ 
    {
        lmode=ParseInteger(strPtr+2,1);//ȡò        
    } 
    if((strPtr=strstr(lcdHmiMsgRec->payload,"MT"))!=NULL)//յַ 
    {
        tmode=ParseInteger(strPtr+2,1);//ȡò        
    } 
    if((strPtr=strstr(lcdHmiMsgRec->payload,"MS"))!=NULL)//յַ 
    {
        smode=ParseInteger(strPtr+2,1);//ȡò        
    } 
    
    if((strPtr=strstr(lcdHmiMsgRec->payload,"LD"))!=NULL)//յַ 
    {
        if(lmode==1)ldw=ParseInteger(strPtr+2,2);//ȡò        
    }       
    if((strPtr=strstr(lcdHmiMsgRec->payload,"TD"))!=NULL)//յַ 
    {
        if(tmode==1)tdw=ParseInteger(strPtr+2,2);//ȡò        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"SD"))!=NULL)//յַ 
    {
        if(smode==1)sdw=ParseInteger(strPtr+2,2);//ȡò        
    }
    if((strPtr=strstr(lcdHmiMsgRec->payload,"HMI"))!=NULL)//յַ     
    {
    }

    limitSaveDataRange();//HMIɺͳһƲΧ
               
}



void OnGetUartMessage(const _uart_msg_obj *uartMsgRec)
{
    char *strPtr;
    u8 buzFlag =0;//ǷҪ
    if((strPtr=strstr(uartMsgRec->payload,"SL"))!=NULL)//յַ 
    {
        lset=ParseInteger(strPtr+2,2);//ȡò       
        buzFlag =1;  //Ҫ      
    }
    if((strPtr=strstr(uartMsgRec->payload,"ST"))!=NULL)//յַ
    {
        tset=ParseInteger(strPtr+2,2);//ȡò  
        buzFlag =1;  //Ҫ              
    }
    if((strPtr=strstr(uartMsgRec->payload,"SC"))!=NULL)//յַ 
    {
        sset=ParseInteger(strPtr+2,2);//ȡò
        limitSaveDataRange();//ֵ99ⱨʧЧ        
        buzFlag =1;  //Ҫ      
    } 

    if((strPtr=strstr(uartMsgRec->payload,"ML"))!=NULL)//յַ 
    {
        lmode=ParseInteger(strPtr+2,1);//ȡò        
        buzFlag =1;  //Ҫ      
    } 
    if((strPtr=strstr(uartMsgRec->payload,"MT"))!=NULL)//յַ 
    {
        tmode=ParseInteger(strPtr+2,1);//ȡò        
        buzFlag =1;  //Ҫ      
    } 
    if((strPtr=strstr(uartMsgRec->payload,"MS"))!=NULL)//յַ 
    {
        smode=ParseInteger(strPtr+2,1);//ȡò        
        buzFlag =1;  //Ҫ      
    } 
    
    if((strPtr=strstr(uartMsgRec->payload,"LD"))!=NULL)//յַ 
    {
        if(lmode==1)ldw=ParseInteger(strPtr+2,2);//ȡò    
        buzFlag =1;  //Ҫ              
    }       
    if((strPtr=strstr(uartMsgRec->payload,"TD"))!=NULL)//յַ 
    {
        if(tmode==1)tdw=ParseInteger(strPtr+2,2);//ȡò        
        buzFlag =1;  //Ҫ      
    }
    if((strPtr=strstr(uartMsgRec->payload,"SD"))!=NULL)//յַ 
    {
        if(smode==1)sdw=ParseInteger(strPtr+2,2);//ȡò        
        buzFlag =1;  //Ҫ      
    }

    if((strPtr=strstr(uartMsgRec->payload,"%"))!=NULL)//յַ
    {
        if(wirelessFlag == 0)USARTSendString(USART1,"\r\nTest Ok!!\r\n\r\n");	 //	 Ӧ 
    }        
    
    limitSaveDataRange();//APPɺͳһƲΧ

    if(buzFlag == 1)//Ҫ
    {
        buzFlag = 0;//ձ־
        My_LEDBlink(BUZ_PIN,1,1,50,100);//Ʒ//       
        needWriteFlash=10;//My_STMFlash_SaveUseSector(saveData);   //ݱ浽flash 粻ʧ                    
    }  
}


void My_ESP8266_SendStrStr(USART_TypeDef* USARTx, const char *str)
{
    USARTSendString(USARTx, str);
}

void checkWireLessMode(u8 recBuf)//
{
    static u8 checkIn = 0;//鿴һα־Ƿ
    if(startCheckWireLess == 1 && wirelessFlag == 0)//
    {
        if(recBuf == 'O')checkIn = 1;
 
    }
}
