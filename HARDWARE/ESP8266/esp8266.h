#ifndef __ESP_H
#define __ESP_H
#include "sys.h"

#define Device_ID	0x0001			//����������

#define USART2_MAX_RECV_LEN		600					//�����ջ����ֽ���
#define USART2_MAX_SEND_LEN		600					//����ͻ����ֽ���
#define USART2_MAX_RECV_LEN		600					//�����ջ����ֽ���
#define USART2_MAX_SEND_LEN		600					//����ͻ����ֽ���
#define USART2_RX_EN 					1						//0,������;1,����.

//�����������豸
#define CMD_ONLINE        0x01      //�������
#define CMD_SET_TIME			0x02			//������λ��ʱ��
#define CMD_OPEN					0x03			//�����ź�
#define CMD_CONNECT				0x04			//��������WiFi

//ʱ��ṹ��
typedef struct
{
    u8 year[4];
    u8 month[2];
    u8 date[2];
    u8 hour[2];
    u8 minute[2];
    u8 second[2];
}TimeTypeDef;
//ʱ������������
typedef union
{
    TimeTypeDef TIME;
    u8 time_arrary[12];
}Union_Time;

extern u8  USART2_RX_BUF[USART2_MAX_RECV_LEN]; 		//���ջ���,���USART3_MAX_RECV_LEN�ֽ�
extern u8  USART2_TX_BUF[USART2_MAX_SEND_LEN]; 		//���ͻ���,���USART3_MAX_SEND_LEN�ֽ�
extern vu16 USART2_RX_STA;   											//��������״̬
extern u8 connect_state;
extern u8 start_flag;

void usart2_Init(u32 bound);	//����2��ʼ��
void u2_printf(char* fmt,...);		//����2�������ݺ���
void esp8266_Init(void);			//esp8266��ʼ��

extern s8 sendAT(char *sendStr,char *searchStr,u32 outTime);//����ATָ���
extern void cleanReceiveData(void);    //�������������
extern char * my_strstr(char *FirstAddr,char *searchStr);	//strstr����
s8 TCP_Server(void);		//���÷�����
void decodeData(void);	//������������Ϣ
void sendBack(u8 CMD_TYPE,ErrorStatus error);	//���ͻظ�Ӧ��

void ClearnSDCache(void);		//���SD����
void SendOnline(void);			//��������Ӧ��
void SetTime(void);					//����ʱ��
void OpenDoor(void);				//����ָ��
void SetNet(void);					//��������WiFi

void switch_CMD(void);			//ָ�����

#endif




