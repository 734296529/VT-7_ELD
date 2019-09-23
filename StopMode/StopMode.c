#include "StopMode.h"
#include "rtc.h"
#include "../Timers/soft_timer.h"
#include "../W25QXX/W25QXX.h"
#include "../OBD_Fun/OBD_Fun.h"
#include "iwdg.h"
#include "usart.h"
#include "tim.h"

void SystemClock_Config(void);

//进入停止模式
void Enter_StopMode(void)
{
/*
	printf("Enter StopMode!\r\n");
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);
	//HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_SLEEPENTRY_WFI);//原工程代码
	SystemClock_Config();
	printf("Exit StopMode!\r\n");
*/

	isStopMode = ON;
	obd_Rdy = 0;
	//睡眠蓝牙、STN1110模块
  HAL_GPIO_WritePin(SLEEP_GPIO_Port, SLEEP_Pin, GPIO_PIN_RESET);

	TIMERS_Stop(0);
//	TIMERS2_Stop(1);
	HAL_GPIO_WritePin(LED_ON_GPIO_Port,LED_ON_Pin,GPIO_PIN_SET);//灯熄灭
	printf(" Enter StopMode!\r\n");
	while(isStopMode)
	{	
		//RTC闹钟每秒中断唤醒去喂狗
		HAL_RTC_SetSecAlarm_IT(&hrtc);

		//进入低功耗模式
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);
		
		HAL_IWDG_Refresh(&hiwdg);//复位看门狗
		printf("\r\n");
	}
	//睡眠模式下，自动切换内部时钟，退出低功耗模式后，需要重新配置时钟频率
	SystemClock_Config();
	//唤醒蓝牙、STN1110模块
  HAL_GPIO_WritePin(SLEEP_GPIO_Port, SLEEP_Pin, GPIO_PIN_SET);
	HAL_Delay(30);	

	isStartUp = 1;//修改发动机状态为点火	
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






