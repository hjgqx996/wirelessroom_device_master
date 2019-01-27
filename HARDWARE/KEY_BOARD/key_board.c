#include "key_board.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "lcd.h"
#include "text.h"
#include "touch.h"
#include "string.h"


//���ֱ�
const u8* kbd_tbl[16]={"1","2","3","��","4","5","6","OFF","7","8","9","OK","*","0","#","�鿴"};

////���ؼ��̽���
//x,y:������ʼ����
void key_load_ui(u16 x,u16 y)
{
	u16 i;
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	LCD_DrawRectangle(x,y-40,x+200,y);
	LCD_DrawRectangle(x,y,x+200,y+160);
	LCD_DrawRectangle(x+50,y,x+100,y+160);
	LCD_DrawRectangle(x+150,y,x+200,y+160);
	LCD_DrawRectangle(x,y+40,x+200,y+80);
	LCD_DrawRectangle(x,y+120,x+200,y+160);

	POINT_COLOR=BLUE;
	for(i=0;i<16;i++)
	{
		Show_Str_Mid(x+(i%4)*50,y+10+40*(i/4),(u8*)kbd_tbl[i],16,60);
	}
	Show_Str(250,20,80,16,(u8 *)"1��101",16,0);
	POINT_COLOR=BLUE;//��������Ϊ��ɫ 
 	LCD_ShowString(230,90,100,16,16,(u8 *)"Temp:  C");
 	LCD_ShowString(230,110,100,16,16,(u8 *)"Humi:  %");
}

//����״̬����
//x,y:��������
//key:��ֵ��0~8��
//sta:״̬��0���ɿ���1�����£�
void key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{
	u16 i=keyx/4,j=keyx%4;
	if(keyx>16)return;
	if(sta)LCD_Fill(x+j*50+1,y+i*40+1,x+j*50+49,y+i*40+39,GREEN);
	else LCD_Fill(x+j*50+1,y+i*40+1,x+j*50+49,y+i*40+39,WHITE); 
	Show_Str_Mid(x+j*50,y+10+40*i,(u8*)kbd_tbl[keyx],16,60);
}
//�õ�������������
//x,y:��������
//����ֵ��������ֵ��1~9��Ч��0,��Ч��
u8 get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x=0;//0,û���κΰ������£�1~9��1~9�Ű�������
	u8 key=0;
	tp_dev.scan(0);
	if(tp_dev.sta&TP_PRES_DOWN)			//������������
	{
		LCD_LED = 1;
		for(i=0;i<4;i++)
		{
			for(j=0;j<4;j++)
			{
			 	if(tp_dev.x[0]<(x+j*50+50)&&tp_dev.x[0]>(x+j*50)&&tp_dev.y[0]<(y+i*40+40)&&tp_dev.y[0]>(y+i*40))
				{
					key=i*4+j+1;
					break;
				}
			}
			if(key)
			{
				if(key_x==key)key=0;
				else
				{
					key_staset(x,y,key_x-1,0);
					key_x=key;
					key_staset(x,y,key_x-1,1);
				}
				break;
			}
		}
	}else if(key_x)
	{
		key_staset(x,y,key_x-1,0);
		key_x=0;
	}
	return key;
}

