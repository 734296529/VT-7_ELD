#include "StopMode.h"
#include "rtc.h"
#include "../Timers/soft_timer.h"
#include "../W25QXX/W25QXX.h"
#include "../OBD_Fun/OBD_Fun.h"
#include "iwdg.h"
#include "usart.h"
#include "tim.h"

void SystemClock_Config(void);

//����ֹͣģʽ
void Enter_StopMode(void)
{
/*
	printf("Enter StopMode!\r\n");
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);
	//HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_SLEEPENTRY_WFI);//ԭ���̴���
	SystemClock_Config();
	printf("Exit StopMode!\r\n");
*/

	isStopMode = ON;
	obd_Rdy = 0;
	//˯��������STN1110ģ��
  HAL_GPIO_WritePin(SLEEP_GPIO_Port, SLEEP_Pin, GPIO_PIN_RESET);

	TIMERS_Stop(0);
//	TIMERS2_Stop(1);
	HAL_GPIO_WritePin(LED_ON_GPIO_Port,LED_ON_Pin,GPIO_PIN_SET);//��Ϩ��
	printf(" Enter StopMode!\r\n");
	while(isStopMode)
	{	
		//RTC����ÿ���жϻ���ȥι��
		HAL_RTC_SetSecAlarm_IT(&hrtc);

		//����͹���ģʽ
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);
		
		HAL_IWDG_Refresh(&hiwdg);//��λ���Ź�
		printf("\r\n");
	}
	//˯��ģʽ�£��Զ��л��ڲ�ʱ�ӣ��˳��͹���ģʽ����Ҫ��������ʱ��Ƶ��
	SystemClock_Config();
	//����������STN1110ģ��
  HAL_GPIO_WritePin(SLEEP_GPIO_Port, SLEEP_Pin, GPIO_PIN_SET);
	HAL_Delay(30);	

	isStartUp = 1;//�޸ķ�����״̬Ϊ���	
	DFL168_Init();
//	TIMERS2_Start(1);
	TIMERS_Start(0);
	obd_Rdy = 1;
	printf(" Exit StopMode!\r\n");
}

void Sleep_Manage(void)
{
	if(RTC_ReadTimeCounter(&hrtc) - sleepCounter >= sleepDelay)
	{
		Enter_StopMode();
	}
}






