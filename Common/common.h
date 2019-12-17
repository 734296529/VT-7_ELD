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

#define MCU_Version	"V1.2.0-J"//��Ƭ���汾��
/* ���ڿɽ�������ַ����� */
#define UARTSIZE 		255
#define PAGE_SIZE		0x100
u8 SPI1_ReadWriteByte(uint8_t TxData);

extern u8 ReceiveBuff[UARTSIZE];	//���ڽ��ջ�����
extern u8 ReceiveBuff2[UARTSIZE];
extern volatile u8 recv_end_flag, Rx_len;		//���ڽ��ջ����жϱ�־,�����ַ�����
extern volatile u8 recv_end_flag2, Rx_len2;

extern u8 dataCache[UARTSIZE];				//�������ݻ�����
extern volatile u16 pgn_Flag;					//PGN��¼

#define OBD_FLASH_START		0x00000000	   
#define OBD_FLASH_END    	0X01FFDFFF

#define ADDR_INDEX_START  0x01FFE000
#define ADDR_INDEX_END    0x01FFEFFF

/* SPI��дFLASH�Ļ��������� */
//extern u8 wSpiData[PAGE_SIZE];
//extern u8 rSpiData[PAGE_SIZE];

extern volatile bool isStopMode;		//˯��ģʽ��־λ
extern volatile u32 sleepDelay;			//Ϩ����ʱ����ʱ��(s)
extern volatile u32 sleepCounter;		//��ǰϨ��ʱ��(s)
extern volatile u32 delayStart;			//��ʱ�ȴ���ʼʱ��
extern volatile u32 delayCounter;		//��ʱ�ȴ�ʱ��
extern volatile u8 delayTimes;			//��ʱ�ȴ�����
extern volatile bool isStartUp;			//���������״̬
extern volatile u8 SyncFlag;				//����ͬ����־λ
extern volatile u8 StoreFlag;	 			//����д���־λ
extern volatile u8 SendFlag;				//�������ݱ�־λ
extern volatile u8 ELD_Rdy;					//��ʼ����ɱ�־ 
extern volatile u8 CMD_Flag;				//�����б�־λ
extern volatile u8 getDTCsFlag;				//��ȡ�����־λ
extern volatile u8 clearDTCsFlag;			//��������־λ
extern volatile u8 recv_OK;					//��λ��Ӧ���־
extern u8 ProtRecvBuff[150];				//��λ��ָ���
extern volatile u8 ProtRecvLen;	  	//��λ��ָ���
extern volatile u32 BackupAddr;			//���ݱ��ݵ�ַ
extern volatile u32 AddrIndex;			//����������ַ
extern volatile u32 SyncAddr;				//����ͬ����ַ
extern volatile u32 SyncAddrNow;		//����ͬ����ǰ��ַ
extern volatile u32 SyncAddrStart;	//����ͬ����ʼ��ַ
extern volatile u32 SyncAddrEnd;		//����ͬ��������ַ

void Delay_ms(int ms);							//�Զ�����ʱ����
void LED_Manage(void);
char charhextoascii(char *hex_byte);// �ַ���16����תASCII��(2λ)
int htoi(char x); //16�����ַ���ת����(���ַ�)
int getSpaceNum(char *str); //�����ַ����пո�����
int checkASCIIRange(char s);//�ж��ַ��ĺϷ���
int strstrcount(char *str1, char *str2);//�����ַ������Ӵ�������
u8 Flash_Check(void);
#endif 
