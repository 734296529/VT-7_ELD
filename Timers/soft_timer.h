/* 
 * File:   soft_timer.h
 *
 * Created on 2018/06/20, 15:53
 */

#ifndef __SOFT_TIMER_H
#define	__SOFT_TIMER_H

#include "../Common/common.h"

#ifdef	__cplusplus
extern "C" {
#endif 
#define uint unsigned int    
#define MAX_SOFT_TIMER_COUNT		4
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
struct timers_t
{
		bool isStart;
    bool isLoop;
    void (*pHandler)();
    u32 count;
		u32 curCount;    
};

void TIMERS_Add(u8 index, u32 count,bool loop, void (*pHandler)());
void TIMERS_Start(u8 index);
void TIMERS_Start_Now(u8 index);
void TIMERS_Stop(u8 index);
void TIMERS_Manager(void);

void TIMERS2_Add(u8 index, u32 count,bool loop, void (*pHandler)());
void TIMERS2_Start(u8 index);
void TIMERS2_Start_Now(u8 index);
void TIMERS2_Stop(u8 index);
void TIMERS2_Manager(void);


void Display_Time(void);

#ifdef	__cplusplus
}
#endif

#endif	/* SOFT_TIMER_H */

