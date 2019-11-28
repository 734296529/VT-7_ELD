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

void ELD_funStart(void);
void ELD_funStop(void);

void J1939_Handler(void);
void sendPGN(void);
void recvPGN(void);
void sendData(void);
int ELD_transData(u8* protocolData);
int ELD_TtoP(OBD_T data_t,u8* data_p);

int getVin(void);
int getVss(void);
int getRpm(void);
int getEngineHours(void);
int getMiles(void);
int getHighResMiles(void);
int getDTCs(void);
int clearDTCs(void);
int autoBaudRate(void);
int setsetBaudRate(int deep);
int baudTest(void);
int setBroadcastMode(bool mode1);

int protHandler(u8* recvCmd,u8 len);
u8 writeCmd(char* cmd);
u8 readCmd(u8* Data,u8* data_len,u32 timeout);
int mcuReply(u16 cmdCode,u8 replyFlag);
int versionReply(void);
int setSleepDelay(u8 slpDly);
int backup_Handler(u8 i);
int backup_2Flash(void);
int setSyncRange(u16 mStart,u16 mEnd);
void sync_Handler(void);
void sync_SendData(u8 mBuf[][32]);
void CMD_Handler(void);
void CMD_Pass(u8 *buf);
#endif
