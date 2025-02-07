#ifndef __COMMON_H_
#define __COMMON_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef volatile unsigned char vu8;
typedef volatile unsigned short int vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

#define MCU_Version	0X06//单片机版本号
/* 串口可接收最大字符个数 */
#define UARTSIZE 		255
#define PAGE_SIZE		0x100
u8 SPI1_ReadWriteByte(uint8_t TxData);

extern u8 ReceiveBuff[UARTSIZE];	//串口接收缓冲区
extern u8 ReceiveBuff2[UARTSIZE];
extern volatile u8 recv_end_flag, Rx_len;		//串口接收缓冲中断标志,接收字符长度
extern volatile u8 recv_end_flag2, Rx_len2;

#define OBD_FLASH_START		0x00000000	  
#define OBD_FLASH_END    	0X01FFDFFF

#define ADDR_INDEX_START  0x01FFE000
#define ADDR_INDEX_END    0x01FFEFFF
//typedef struct backupaddr_t
//{
//	u8 isLooped;
//	u32 addr;
//}BACKUPADDR_T;
//extern BACKUPADDR_T BackupAddr_t;
/* SPI读写FLASH的缓冲区变量 */
//extern u8 wSpiData[PAGE_SIZE];
//extern u8 rSpiData[PAGE_SIZE];

extern volatile bool isStopMode;		//睡眠模式标志位
extern volatile u32 sleepDelay;			//熄火延时休眠时间(s)
extern volatile u32 sleepCount;			//当前熄火时间(s)
extern volatile u8 backupSwitch;		//当前存储开关
extern volatile bool isStartUp;			//发动机点火状态
extern volatile u8 SyncFlag;				//数据同步标志位
extern volatile u8 StoreFlag;	 			//数据写入标志位
extern volatile u8 obd_Rdy;			

extern volatile u8 recv_OK;					//上位机应答标志
extern u8 ProtRecvBuff[150];				//上位机指令缓存
extern volatile u8 ProtRecvLen;	  	//上位机指令长度
extern volatile u32 BackupAddr;			//数据备份地址
extern volatile u32 AddrIndex;			//备份索引地址
extern volatile u32 SyncAddr;				//数据同步地址
extern volatile u32 SyncAddrNow;		//数据同步当前地址
extern volatile u32 SyncAddrStart;	//数据同步起始地址
extern volatile u32 SyncAddrEnd;		//数据同步结束地址

void Delay_ms(int ms);							//自定义延时函数
void LED_Manage(void);
char charhextoascii(char *hex_byte);// 字符串16进制转ASCII码(2位)
int htoi(char x); //16进制字符串转整形(单字符)
int getSpaceNum(char *str); //计算字符串中空格数量
int checkASCIIRange(char s);//判断字符的合法性
u8 Flash_Check(void);
#endif 

