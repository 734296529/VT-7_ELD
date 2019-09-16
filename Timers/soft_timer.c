#include "soft_timer.h"
#include "usart.h"
#include <string.h>
#include "stm32f1xx_hal_pwr.h"
#include "rtc.h"
#include "time.h"

void SystemClock_Config(void);
struct timers_t soft_timers[MAX_SOFT_TIMER_COUNT] = {0};
struct timers_t soft_timers2[MAX_SOFT_TIMER_COUNT] = {0};

/* 定时器1软定时器管理 */
void TIMERS_Start(u8 index)
{
	soft_timers[index].isStart = 1;
	soft_timers[index].curCount = soft_timers[index].count;
}

void TIMERS_Start_Now(u8 index)
{
	soft_timers[index].isStart = 1;
	soft_timers[index].curCount = 1;
}

void TIMERS_Stop(u8 index)
{
	soft_timers[index].isStart = 0;
}

void TIMERS_Add(u8 index, u32 count,bool loop, void (*pHandler)())
{
	soft_timers[index].count = count;
	soft_timers[index].curCount = count;
	soft_timers[index].pHandler = pHandler;
	soft_timers[index].isStart = 0;
	soft_timers[index].isLoop = loop;
}

void TIMERS_Manager(void)
{
	u32 i;
	for(i=0;i<MAX_SOFT_TIMER_COUNT;i++)
	{
		soft_timers[i].curCount --;
		if(soft_timers[i].curCount == 0 && soft_timers[i].isStart == 1)
		{			
				if(soft_timers[i].pHandler){
						soft_timers[i].pHandler();
				}
				if(soft_timers[i].isLoop){
						soft_timers[i].curCount = soft_timers[i].count;
				}else{
						soft_timers[i].isStart = 0;
				}
 		}			
	}
}

/* 定时器2软定时器管理 */
void TIMERS2_Start(u8 index)
{
	soft_timers2[index].isStart = 1;
	soft_timers2[index].curCount = soft_timers2[index].count;
}

void TIMERS2_Start_Now(u8 index)
{
	soft_timers2[index].isStart = 1;
	soft_timers2[index].curCount = 1;
}

void TIMERS2_Stop(u8 index)
{
	soft_timers2[index].isStart = 0;
}

void TIMERS2_Add(u8 index, u32 count,bool loop, void (*pHandler)())
{
	soft_timers2[index].count = count;
	soft_timers2[index].curCount = count;
	soft_timers2[index].pHandler = pHandler;
	soft_timers2[index].isStart = 0;
	soft_timers2[index].isLoop = loop;
}

void TIMERS2_Manager(void)
{
	u32 i;
	for(i=0;i<MAX_SOFT_TIMER_COUNT;i++)
	{
		soft_timers2[i].curCount --;
		if(soft_timers2[i].curCount == 0 && soft_timers2[i].isStart == 1)
		{			
				if(soft_timers2[i].pHandler){
						soft_timers2[i].pHandler();
				}
				if(soft_timers2[i].isLoop){
						soft_timers2[i].curCount = soft_timers2[i].count;
				}else{
						soft_timers2[i].isStart = 0;
				}
 		}			
	}
}


//打印时间
void Display_Time(void)
{
//	RTC_DateTypeDef sdatestructure;
//	RTC_TimeTypeDef stimestructure;

//	/* 获取当前RTC时间，必须先获取时间*/
//	HAL_RTC_GetTime(&hrtc, &stimestructure, RTC_FORMAT_BIN);
//	/* 获取当前RTC日期 */
//	HAL_RTC_GetDate(&hrtc, &sdatestructure, RTC_FORMAT_BIN);
//	
//	/* 打印日期,格式 : yy/mm/dd */
//	printf("%02d/%02d/%02d",2000 + sdatestructure.Year, sdatestructure.Month, sdatestructure.Date); 
//	/* 打印时间,格式 : hh:mm:ss */
//	printf(" %02d:%02d:%02d\r\n",stimestructure.Hours, stimestructure.Minutes, stimestructure.Seconds);	
	
//	struct tm stm;  
//	time_t Unix;
//	//RTC时间转换格式为系统时间格式
//	memset(&stm,0,sizeof(stm));  
//	stm.tm_year = sdatestructure.Year+100;
//	stm.tm_mon  = sdatestructure.Month-1;  
//	stm.tm_mday = sdatestructure.Date;  
//	stm.tm_hour = stimestructure.Hours;  
//	stm.tm_min  = stimestructure.Minutes;  
//	stm.tm_sec  = stimestructure.Seconds;
//	//系统时间转换为时间戳
//	Unix=mktime(&stm);

//	/* 打印Unix时间戳 */
//	printf("Unix:%d\r\n",Unix);
}


