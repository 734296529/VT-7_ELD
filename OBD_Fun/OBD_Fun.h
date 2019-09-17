#ifndef __ELD_FUN_H_
#define __ELD_FUN_H_
#include "stm32f1xx_hal.h"
#include "../Common/common.h"

typedef struct obd_t
{
	u8 length;
	u8 type;
	u8 Data[24];
}OBD_T;

int DFL168_Init(void);

void OBD_funStart(void);
void OBD_funStop(void);
void OBD_Run(void);

void J1939_getData(void);
int OBD_transData(u8* protocolData);
int OBD_TtoP(OBD_T data_t,u8* data_p);

//int OBD_backupData(void);
//int OBD_backupFun(u8* protocolData);
int OBD_sendBackup(void);
int OBD_transBackup(u8* data,u32 addr);
//int OBD_refreshBackupAddr(u32 addr);

int getVin(void);
int getVss(void);
int getRpm(void);
int getEngineHours(void);
int getMiles(void);
int setsetBaudRate(int deep);
int setBroadcastMode(bool mode1);

int protHandler(u8* recvCmd,u8 len);
u8 writeCmd(char* cmd);
u8 readCmd(u8* Data,u8* d