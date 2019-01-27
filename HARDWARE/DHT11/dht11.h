#ifndef __DHT11_H
#define __DHT11_H 
#include "sys.h"   
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//ALIENTEK STM32F407������
//DHT11 ��������
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/5/7
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) �������������ӿƼ����޹�˾ 2014-2024
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////

//IO��������
#define DHT11_IO_IN()  {GPIOA->MODER&=~(3<<(6*2));GPIOA->MODER|=0<<6*2;}	//PA6����ģʽ
#define DHT11_IO_OUT() {GPIOA->MODER&=~(3<<(6*2));GPIOA->MODER|=1<<6*2;} 	//PA6���ģʽ 
////IO��������											   
#define	DHT11_DQ_OUT PAout(6) //���ݶ˿�	PA6 
#define	DHT11_DQ_IN  PAin(6)  //���ݶ˿�	PA6 

u8 DHT11_Init(void);//��ʼ��DHT11
u8 DHT11_Read_Data(u8 *temp,u8 *humi);//��ȡ��ʪ��
u8 DHT11_Read_Byte(void);//����һ���ֽ�
u8 DHT11_Read_Bit(void);//����һ��λ
u8 DHT11_Check(void);//����Ƿ����DHT11
void DHT11_Rst(void);//��λDHT11
#endif














