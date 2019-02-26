/*									Include Head File														*/
#include "esp8266.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "my_math.h"
#include "usart.h"
#include "lcd.h"
#include "delay.h"
#include "ff.h"
#include "rtc.h"
#include "malloc.h"
#include "led.h"
#include "text.h"

FIL fdsts_recive;
UINT readnum;
u8 connect_state;		//����������״̬��0�������ӣ�1���Ͽ�
u8 start_flag = 0;

//���ڽ��ջ�����
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 			//���ջ���,���USART2_MAX_RECV_LEN���ֽ�.
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN]; 			//���ͻ���,���USART2_MAX_SEND_LEN�ֽ�

//ͨ���жϽ�������2���ַ�֮���ʱ������10ms�������ǲ���һ������������.
//���2���ַ����ռ������10ms,����Ϊ����1����������.Ҳ���ǳ���10msû�н��յ�
//�κ�����,���ʾ�˴ν������.
//���յ�������״̬
//[15]:0,û�н��յ�����;1,���յ���һ������.
//[14:0]:���յ������ݳ���
vu16 USART2_RX_STA=0;

void usart2_Init(u32 bound)
{
    NVIC_InitTypeDef NVIC_InitStructure;
		GPIO_InitTypeDef GPIO_InitStructure;
		USART_InitTypeDef USART_InitStructure;
		
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //ʹ��GPIOBʱ��
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//ʹ��USART3ʱ��

    USART_DeInit(USART2);  //��λ����2
	
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2); //GPIOB11����ΪUSART3
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2); //GPIOB10����ΪUSART3	
		
    //USART2_TX   PA2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PB10
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
		GPIO_Init(GPIOA,&GPIO_InitStructure); //��ʼ��GPIOB11����GPIOB10

    //USART2_RX	  PA3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
		GPIO_Init(GPIOA,&GPIO_InitStructure); //��ʼ��GPIOB11����GPIOB10

    USART_InitStructure.USART_BaudRate = bound;//������һ������Ϊ9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
    USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

    USART_Init(USART2, &USART_InitStructure); //��ʼ������	2

    USART_Cmd(USART2, ENABLE);                    //ʹ�ܴ���

    //ʹ�ܽ����ж�
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//�����ж�

    //�����ж����ȼ�
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//�����ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

    USART2_RX_STA=0;		//����

}

//����2,printf ����
//ȷ��һ�η������ݲ�����USART2_MAX_SEND_LEN�ֽ�
void u2_printf(char* fmt,...)
{
    u16 i,j;
    va_list ap;
    va_start(ap,fmt);
    vsprintf((char*)USART2_TX_BUF,fmt,ap);
    va_end(ap);
    i=strlen((const char*)USART2_TX_BUF);		//�˴η������ݵĳ���
    for(j=0; j<i; j++)							//ѭ����������
    {
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET); //ѭ������,ֱ���������
        USART_SendData(USART2,USART2_TX_BUF[j]);
    }
}

u8 msg_flag;
//����2�жϷ�����
void USART2_IRQHandler(void)
{
	u8 res;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//���յ�����
	{
		res =USART_ReceiveData(USART2);
		if((USART2_RX_STA&(1<<15))==0)//�������һ������,��û�б�����,���ٽ�����������
		{
			if(start_flag==0)
			{
				if(USART2_RX_STA<USART2_MAX_RECV_LEN)
				{
					USART2_RX_BUF[USART2_RX_STA++] = res;
				}
			}
			else if(start_flag==1)
			{
				if(res=='@')
				{
					USART2_RX_STA&=0x0000;
					msg_flag = 1;
				}
				if(USART2_RX_STA<USART2_MAX_RECV_LEN)
				{
					USART2_RX_BUF[USART2_RX_STA++] = res;
//					USART2_RX_STA++;
				}
			}
		}
	}
}

//����һ��ATָ�������Ƿ��յ�ָ����Ӧ��
//*sndStr:���͵������ַ���,��sendStr<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//*searchStr:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//outTime:�ȴ�ʱ��(��λ:1ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       -1,����ʧ��
s8 sendAT(char *sendStr,char *searchStr,u32 outTime)
{
    u16 i;
    s8 ret = 0;
    char * res =0;
//	outTime=outTime/10;
    cleanReceiveData();//���������
    if((u32)sendStr < 0xFF)
    {
        while((USART2->SR&0X40)==0);//�ȴ���һ�����ݷ������
        USART2->DR=(u32)sendStr;
    }
    else
    {
        u2_printf(sendStr);//����ATָ��
        u2_printf("\r\n");//���ͻس�����
    }
		delay_us(500);
    if(searchStr && outTime)//��searchStr��outTime��Ϊ0ʱ�ŵȴ�Ӧ��
    {
        while((--outTime)&&(res == 0))//�ȴ�ָ����Ӧ���ʱʱ�䵽��
        {
            res = my_strstr((char *)USART2_RX_BUF,searchStr);
            if(res!=0)
                break;
            if((i==USART2_MAX_RECV_LEN)||res!=0)		//i���ã�����
                break;
            delay_ms(1);
        }
        if(outTime == 0)
        {
            ret = -1;    //��ʱ
        }
        if(res != 0)//res��Ϊ0֤���յ�ָ��Ӧ��
        {
            ret = 0;
        }
    }
		printf("%s",USART2_RX_BUF);
    delay_ms(50);
		cleanReceiveData();		//��ս������ݻ���
    return ret;
}

//���������
//��������
//����ֵ ��
void cleanReceiveData(void)
{
    u16 i;
    USART2_RX_STA=0;			//���ռ���������
    for(i = 0; i < USART2_MAX_RECV_LEN; i++)
    {
        USART2_RX_BUF[i] = 0;
    }
}

//�ڴ��ڽ��յ��ַ���������
//����ֵ		�ɹ����ر������ַ������׵�ַ
//				ʧ�ܷ��� 0
//��һ����BUG���ڽ��������д���0������ֹͣ����
char * my_strstr(char *FirstAddr,char *searchStr)
{

    char * ret = 0;

//    if((u3_data_Pack.USART1_RX_STA&(1<<15))!=0)	//�������
//    {
//    if((u3_data_Pack.USART3_RX_STA|(0x8FFF)) >= USART3_MAX_RECV_LEN)
//    {
//        u3_data_Pack.USART3_RX_BUF[USART3_MAX_RECV_LEN-1] = '\0';
//    }
    ret = strstr((char *)FirstAddr,searchStr);
    if(ret != 0)
    {
        return ret;
    }
//    }

    return 0;
}

//�������״̬
void Check_Status(void)
{
		u8 state;
		printf("��ʼ�������״̬\r\n");
		if(sendAT("AT","OK",1000)==0)
		{
				u2_printf("AT+CIPSTATUS\r\n");
				delay_ms(25);
				printf("%s",USART2_RX_BUF);
				printf("%c",USART2_RX_BUF[7]);
				state = USART2_RX_BUF[7] - '0';
				printf("����״̬��%d\r\n",state);
				switch(state)
				{
						case 2:	
							printf("������·������δ���ӷ�����\r\n");
							connect_state = 1;
							LED0 = 1;
						break;
						case 3:
							printf("�����ӷ�����\r\n");
							connect_state = 0;
						break;
						case 4:
							printf("��������Ͽ�����\r\n");
							connect_state = 1;
							LED0 = 1;
						break;
						case 5:
							printf("��·�����Ͽ�����\r\n");
							connect_state = 2;
							LED0 = 1;
						break;
						default:
							printf("δ֪����\r\n");
							connect_state = 3;
							LED0 = 1;
						break;
				}
		}
		else 
		{
				printf("ESP8266ģ���쳣\r\n");
				state = 4;
		}
		printf("����״̬������\r\n");
		cleanReceiveData();
}


char SSID[] = {"Kiven"};		//·����SSID
char password[] = {"asd123456"};	//·��������
//char ipaddr[]= {"111.231.90.29"};//IP��ַ
char ipaddr[] = {"47.100.28.6"};
//char ipaddr[]= {"172.20.10.3"};//IP��ַ
char port[]= {"8086"};				//�˿ں�

void esp8266_reinit(u8 state)
{
		char past[50];
		switch(state)
		{
				case 4:
					sendAT("AT+RST","ready",3000);
				case 3:
					sendAT("AT+RST","ready",3000);
					sendAT("AT","OK",1000);
				case 2:
					if(sendAT("AT+CWMODE=1","OK",1000)==0)
					{
						printf("����ΪSTAģʽ\r\n");
					}
					sprintf(past,"AT+CWJAP_DEF=\"%s\",\"%s\"",SSID,password);
					sendAT(past,"OK",2000);
				case 1:
					sprintf((char *)past,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipaddr,port);
					
					if(sendAT(past,"OK",2000)==0)
					{
							printf("���������ӳɹ�\r\n");
							LED0 = 0;
							connect_state = 0;
							sprintf(past,"@D1%04d",Device_ID);
							u2_printf("+++");
							delay_ms(25);
							u2_printf("AT+CIPMODE=1\r\n");
							delay_ms(15);
							u2_printf("AT+CIPSEND\r\n");
							delay_ms(15);
							u2_printf("%s\r\n",past);
							delay_ms(25);
							u2_printf("+++");
							delay_ms(15);
					}
					else
					{
						printf("����������ʧ��\r\n");
						LED0 = 1;
						connect_state = 1;
					}
				case 0:
					printf("�����������ɹ�\r\n");
				break;
		}
		cleanReceiveData();
}

//ESP8266ģ���ʼ��
void esp8266_Init(void)
{
//    u8 i;
//    s8 ret;
    char past[50];			//·������Ϣ
    usart2_Init(115200);	//��ʼ������2������Ϊ9600
    cleanReceiveData();		//��ս������ݻ���
    sendAT("AT+RST","ready",3000);
		LED0 = 1;
    delay_ms(1000);
		LED0 = 0;
		printf("��ʼ��ʼ��\r\n");
    delay_ms(1000);		//�ȴ�ģ���ϵ��ȶ�
    
    printf("��ʼ���ɹ�\r\n");
		LCD_ShowString(20,20,200,16,16,"ESP8266 init success!");
		LED0 = 1;
		delay_ms(1000);		//�ȴ�ģ���ϵ��ȶ�
		sendAT("ATE0","OK",1000);
		if(sendAT("ATE0","OK",1000)==0)
		{
			printf("�رջ��Գɹ�\r\n");
		}
		if(sendAT("AT+CWMODE=1","OK",1000)==0)
		{
			printf("����ΪSTAģʽ\r\n");
		}
		
		LCD_ShowString(20,40,200,16,16,(u8 *)"ESP8266 set at STA mode");
		
		printf("����·�����ɹ�\r\n");
		LCD_ShowString(20,60,200,16,16,(u8 *)"Connected router Succese");

		sprintf((char *)past,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipaddr,port);

		if(sendAT(past,"OK",2000)==0)
		{
				printf("���������ӳɹ�\r\n");
				LED0 = 0;
				LCD_ShowString(20,80,200,16,16,(u8 *)"Server connected");
				connect_state = 0;
				sprintf(past,"@D1%04d",Device_ID);
				u2_printf("+++");
				delay_ms(25);
				u2_printf("AT+CIPMODE=1\r\n");
				delay_ms(15);
				u2_printf("AT+CIPSEND\r\n");
				delay_ms(15);
				u2_printf("%s\r\n",past);
				delay_ms(25);
				u2_printf("+++");
				delay_ms(15);
		}
		else
		{
			printf("����������ʧ��\r\n");
			LED0 = 1;
			LCD_ShowString(20,80,200,16,16,(u8 *)"Server disconnected");
			connect_state = 1;
		}
		delay_ms(1000);
		cleanReceiveData();		//��ս������ݻ���
		LCD_Clear(WHITE);
}

/*********************************************************************
*��    �ܣ�������������
*��ڲ�����
*���ڲ�����
*********************************************************************/
void decodeData(void)
{
		u8 i,j;
		int strlen = -1;	//���ݳ���
		u8 strbuf[100];		//���ݻ���
		for(i=0;USART2_RX_BUF[i]!='\0';i++)
		{
				//�ҵ�����ͷ
				if(USART2_RX_BUF[i]=='@'&&strlen<0)
				{
						strlen = i;
//						strlen = 0;
//						for(;strlen<USART2_RX_STA;strlen++)
//						{
//								str[strlen]=USART2_RX_BUF[strlen+i];
//						}
//						printf();
				}
//				if(strlen>=0)
//				{
//					strbuf[strlen++] = USART2_RX_BUF[strlen+i];
//				}
		}
		printf("strlen=%d\r\n",strlen);
		for(j=0;j<50;j++)
		{
				strbuf[j] = USART2_RX_BUF[j+strlen];
		}
		printf("j=%d\r\n",j);
		printf("strbuf:%s\r\n",strbuf);
		if(j!=0 && strlen>=0)
		{
				if(f_open(&fdsts_recive,"Receive.txt",FA_WRITE|FA_CREATE_ALWAYS)==FR_OK)
				{
						f_lseek(&fdsts_recive,0);                         				 //�ƶ��ļ�ָ��
						f_write(&fdsts_recive,strbuf,j,&readnum);		 //����
						f_close(&fdsts_recive);
						printf("�ɹ�д������\r\n");
				}
				switch_CMD();
		}
		else
		{
				LED1 = 1;
				delay_ms(50);
				LED1 = 0;
				delay_ms(50);
				LED1 = 1;
		}
		cleanReceiveData();		//��ս������ݻ���
}

/*********************************************************************
*��    �ܣ����ͻظ�Ӧ��
*��ڲ�����
*���ڲ�����
*********************************************************************/
void sendBack(u8 CMD_TYPE,u8 error)
{
		char past[50];
		char date[20];
		memset(past,0,50);
		memset(date,0,20);
		sprintf(past,"@D%1d%04d%1d",CMD_TYPE,Device_ID,error);
//		sprintf(date,"%4d-%02d-%02d %02d:%02d:%02d",calendar.w_year,
//																								calendar.w_month,
//																								calendar.w_date,
//																								calendar.hour,
//																								calendar.min,
//																								calendar.sec);
		u2_printf("AT+CIPMODE=1\r\n");
		delay_ms(15);
		u2_printf("AT+CIPSEND\r\n");
		delay_ms(15);
		printf("data:%s",past);
		u2_printf("%s\r\n",past);
		delay_ms(25);
		u2_printf("+++");
		delay_ms(15);
}

/*********************************************************************
*��    �ܣ����SD��������Ϣ
*��ڲ�����
*���ڲ�����
*********************************************************************/
void ClearnSDCache(void)
{
    if(f_open(&fdsts_recive,"Receive.txt",FA_WRITE)==FR_OK)
    {
        f_lseek(&fdsts_recive,0);           //�ƶ��ļ�ָ��
        f_truncate(&fdsts_recive);			//�ض��ļ�
        f_close(&fdsts_recive);
    }
}

/*********************************************************************
*��    �ܣ���������Ӧ��
*��ڲ�����
*���ڲ�����
*********************************************************************/
void SendOnline(void)
{
		u8 device_id[6];
		u8 device = 0;
		memset(device_id,0,6);
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
    {
        f_lseek(&fdsts_recive,3);                          			//�ƶ��ļ�ָ��
        f_read(&fdsts_recive,device_id,4,&readnum);	//��ȡ�趨ʱ��
        f_close(&fdsts_recive);
    }
//		printf("device_id:%s\r\n",device_id);
		for(u8 i=0;i<4;i++)
		{
				device_id[i] = device_id[i]-48;
				device = device*10+device_id[i];
		}
		printf("%04d",device);
		if(device==Device_ID)
		{
				LED1 = 1;
				ClearnSDCache();         /*���SD��������Ϣ*/
				sendBack(CMD_ONLINE,1);
		}
		else
		{
				LED1 = 1;
				delay_ms(50);
				LED1 = 0;
				delay_ms(50);
				LED1 = 1;
		}
}

/*********************************************************************
*��    �ܣ�����ʱ��
*��ڲ�����
*���ڲ�����
*ʱ�䣺2015��9��18�� 22:12:15
*********************************************************************/
void SetTime(void)
{
		u16 year;
		u8 CMD;
		u8 mon,day,hour,min,sec,week;
		ErrorStatus error;
		u8 device_id[6];
		u8 device = 0;
		Union_Time Gettime;
		memset(Gettime.time_arrary,0,14);
		memset(device_id,0,6);	//����
		//��ȡ�豸��
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
    {
        f_lseek(&fdsts_recive,3);                   //�ƶ��ļ�ָ��
        f_read(&fdsts_recive,device_id,4,&readnum);	//��ȡ�趨ʱ��
        f_close(&fdsts_recive);
    }
		//��ȡ��������
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
    {
        f_lseek(&fdsts_recive,21);              //�ƶ��ļ�ָ��
        f_read(&fdsts_recive,&CMD,1,&readnum);	//��ȡ�趨ʱ��
        f_close(&fdsts_recive);
    }
		CMD -= '0';
//		printf("device_id:%s\r\n",device_id);
		for(u8 i=0;i<4;i++)
		{
				device_id[i] = device_id[i]-48;
				device = device*10+device_id[i];
		}
		printf("%04d",device);
		if(device==Device_ID)
		{
				if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
				{
						f_lseek(&fdsts_recive,7);                          		//�ƶ��ļ�ָ��
						f_read(&fdsts_recive,Gettime.time_arrary,14,&readnum);	//��ȡ�趨ʱ��
						f_close(&fdsts_recive);
				}
				printf("%s\r\n",Gettime.time_arrary);
				for(u8 i=0;i<14;i++)
				{
						Gettime.time_arrary[i] -= 48;
				}
				
				year=(Gettime.TIME.year[0])*1000;     			//����ʱ��
				year+=(Gettime.TIME.year[1])*100;
				year+=(Gettime.TIME.year[2])*10;
				year+=(Gettime.TIME.year[3]);

				mon=(Gettime.TIME.month[0])*10;
				mon+=(Gettime.TIME.month[1]);
			
				day=(Gettime.TIME.date[0])*10;
				day+=(Gettime.TIME.date[1]);
				
				week = RTC_Get_Week(year,mon,day);
			
				hour=(Gettime.TIME.hour[0])*10;
				hour+=(Gettime.TIME.hour[1]);
			
				min=(Gettime.TIME.minute[0])*10;
				min+=(Gettime.TIME.minute[1]);
			
				sec=(Gettime.TIME.second[0])*10;
				sec+=(Gettime.TIME.second[1]);
				
				if(year>2018&&mon<13&&day<32&&hour<24&&min<60&&sec<60)
				{
						switch(CMD)
						{
							case 1:
							{
									error = RTC_Set_Time(hour,min,sec,RTC_H12_AM);	//����ʱ��
									printf("set-error:%d\r\n",error);
									error = RTC_Set_Date(year-2000,mon,day,week);		//��������
									printf("set-error:%d\r\n",error);
									
									RTC_Get();
									LED1 = 1;
									sendBack(CMD_SET_TIME,error);		//����ʱ������Ӧ��
									break;
							}
							case 2:
							{
									printf("��������A\r\n");
									RTC_Set_AlarmA(week,hour,min,sec);
									sendBack(CMD_SET_TIME,error);		//����ʱ������Ӧ��
									break;
							}
						}
				}
		}
		else
		{
				LED1 = 1;
				delay_ms(50);
				LED1 = 0;
				delay_ms(50);
				LED1 = 1;
				sendBack(CMD_SET_TIME,0);		//����ʱ������Ӧ��
		}
		ClearnSDCache();            								//���SD��������Ϣ
}

/*********************************************************************
*��    �ܣ�����ָ��
*��ڲ�����
*���ڲ�����
*********************************************************************/
void OpenDoor(void)
{
		u8 device_id[6];
		u8 userid[12];
		u32 device = 0;
		u8 CMD=0;
		memset(device_id,0,6);
		memset(userid,0,12);
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
		{
				f_lseek(&fdsts_recive,3);                         //�ƶ��ļ�ָ��
				f_read(&fdsts_recive,&device_id,4,&readnum);						//��ȡָ��
				f_close(&fdsts_recive);
		}
//		printf("%s\r\n",device_id);
		for(u8 i=0;i<4;i++)
		{
				device_id[i] = device_id[i]-48;
				device = device*10+device_id[i];
		}
		printf("deviceid:%04d\r\n",device);
		if(device==Device_ID)
		{
				printf("��ַƥ��ɹ�\r\n");
				if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
				{
						f_lseek(&fdsts_recive,18);                         //�ƶ��ļ�ָ��
						f_read(&fdsts_recive,&CMD,1,&readnum);						//��ȡָ��
						f_close(&fdsts_recive);
				}
				CMD -= '0';
				TIM_SetCounter(TIM3, 0);
				LCD_LED = 1;
				printf("CMD:%d\r\n",CMD);
				switch(CMD)
				{
						case 0:		//��������
						{
								if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
								{
										f_lseek(&fdsts_recive,7);                         //�ƶ��ļ�ָ��
										f_read(&fdsts_recive,&userid,11,&readnum);						//��ȡָ��
										f_close(&fdsts_recive);
								}
								DS0 = 1;
								DS1 = 1;
								DR0 = 0;
								TIM_Cmd(TIM4, ENABLE);
								printf("userid:%s\r\n",userid);
								printf("��������\r\n");
								LCD_Fill(230,150,320,180,WHITE);
								Show_Str(230,150,100,16,(u8 *)"��ӭ���٣�",16,0);
								sendBack(CMD_OPEN,0);
								break;
						}
						case 1:		//�������1--δע��
						{
								printf("��δע��\r\n");
								LCD_Fill(230,150,320,180,WHITE);
								Show_Str(230,150,100,16,(u8 *)"��δע�ᣡ",16,0);
								sendBack(CMD_OPEN,1);
								break;
						}
						case 2:		//�������2--δԤԼ
						{
								printf("��δԤԼ\r\n");
								LCD_Fill(230,150,320,180,WHITE);
								Show_Str(230,150,100,16,(u8 *)"��δԤԼ��",16,0);
								sendBack(CMD_OPEN,2);
								break;
						}
						case 3:		//�������3--δ��ԤԼʱ��
						{
								if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
								{
										f_lseek(&fdsts_recive,7);                         //�ƶ��ļ�ָ��
										f_read(&fdsts_recive,&userid,11,&readnum);						//��ȡָ��
										f_close(&fdsts_recive);
								}
								printf("userid:%s\r\n",userid);
								printf("δ��ԤԼʱ��\r\n");
								LCD_Fill(230,150,320,180,WHITE);
								Show_Str(230,150,100,16,(u8 *)"δ��ԤԼʱ�䣡",16,0);
								sendBack(CMD_OPEN,3);
								break;
						}
						case 4:		//�������4--��֤�����
						{
								if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
								{
										f_lseek(&fdsts_recive,7);                         //�ƶ��ļ�ָ��
										f_read(&fdsts_recive,&userid,11,&readnum);						//��ȡָ��
										f_close(&fdsts_recive);
								}
								printf("userid:%s\r\n",userid);
								printf("ԤԼ�ѹ���\r\n");
								LCD_Fill(230,150,320,180,WHITE);
								Show_Str(230,150,100,16,(u8 *)"ԤԼ��ʧЧ��",16,0);
								sendBack(CMD_OPEN,4);
								break;
						}
						default:break;
				}
				LED1 = 1;
		}
		else
		{
				LED1 = 1;
				delay_ms(50);
				LED1 = 0;
				delay_ms(50);
				LED1 = 1;
				sendBack(CMD_OPEN,5);
		}
}

/*********************************************************************
*��    �ܣ���������WiFi
*��ڲ�����
*���ڲ�����
*********************************************************************/
void SetNet(void)
{
		u8 device_id[6];
		u32 device = 0;
		memset(device_id,0,4);
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
		{	 	
				f_lseek(&fdsts_recive,3);                         //�ƶ��ļ�ָ��
				f_read(&fdsts_recive,&device_id,4,&readnum);						//��ȡָ��
				f_close(&fdsts_recive);
		}
		printf("%s\r\n",device_id);
		for(u8 i=0;i<4;i++)
		{
				device_id[i] = device_id[i]-48;
				device = device*10+device_id[i];
		}
		printf("deviceid:%4d\r\n",device);
		if(device == Device_ID)
		{
				LED0 = 1;
				sendBack(CMD_CONNECT,1);
		}
		else
		{
				LED1 = 1;
				delay_ms(50);
				LED1 = 0;
				delay_ms(50);
				LED1 = 1;
				sendBack(CMD_CONNECT,0);
		}
}

/*********************************************************************
*��    �ܣ�ָ��ѡ��
*��ڲ�����
*���ڲ�����
*********************************************************************/
void switch_CMD(void)
{
    u8 CMD=0;
    if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
    {
        f_lseek(&fdsts_recive,2);                         //�ƶ��ļ�ָ��
        f_read(&fdsts_recive,&CMD,1,&readnum);						//��ȡָ��
        f_close(&fdsts_recive);
    }
		CMD -= '0';
//	CMD=CMD_DELETE_PHOTO;
    switch(CMD)			//ָ��ѡ��
    {
				case CMD_ONLINE:		{	SendOnline();	break;	}	//��������Ӧ��
				case CMD_SET_TIME:	{	SetTime();		break;	}	//����ʱ��
				case CMD_OPEN:			{	OpenDoor();		break;	}	//����ָ��
				case CMD_CONNECT:		{ SetNet();			break;	}	//�л�����WiFi
//        case CMD_ADD_USER: 			{   adduser();  		 			break;	} //����û�
//        case CMD_DELETE_USER: 	{   deleteuse(); 		 			break; 	}	//ɾ��ָ���û�
//        case CMD_ONLINE: 				{   Sendonline(); 		 		break; 	}	//������������Ӧ��
//        case CMD_GET_USER_LIST: {   Uploaduserlist(); 	 	break; 	}	//��ȡ��λ���û��б�
//        case CMD_GET_ALL_LIST:  {   Uploadalluserlist();	break;	}	//��ȡ��λ���û�ȫ����Ϣ
//        case CMD_GET_USER_NUM:	{	 	Uploadusernum();	 		break;	}	//��ȡ�û�����
//        case CMD_SET_TIME :			{   settime();			 			break; 	}	//����ʱ��
//        case CMD_GTE_I_O : 			{   Upload_access();	 		break; 	}	//�ϴ����˽�������Ϣ
//				case CMD_SAVE_PHOTO:		{	 	Save_photo();		 			break;	}	//�洢��Ƭ
//				case CMD_DELETE_PHOTO:	{	 	deletephoto();		 		break;	}	//ɾ����Ƭ
//				case CMD_MEMBER:				{	 	new_member();		 			break;	}	//�½���Ա��Ϣ
//				case CMD_CHECK_PHOTO:		{	 	check_photo();		 		break;	}	//��ѰͼƬ�Ƿ����

        default:
				{
						LED1 = 1; 
						delay_ms(50);
						LED1 = 0; 
						delay_ms(50);	
						LED1 = 1; 
						delay_ms(50);
						sendBack(CMD_ONLINE,1);
						break;
				}
    }
		ClearnSDCache();            								//���SD��������Ϣ
}


