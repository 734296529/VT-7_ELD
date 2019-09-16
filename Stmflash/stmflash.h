#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "stm32f1xx_hal.h"
#include "../Common/common.h"

/* Define the APP start address -------------------------------*/
#define ApplicationAddress    0x8005000
#define iapxaddr 0x8000000

/* IAP command------------------------------------------------ */
#define IAP_FLASH_FLAG_ADDR   (uint32_t)(ApplicationAddress - 1024 * 2)//App�����Bootloader��������Ϣ�ĵ�ַ(�ݶ���СΪ2K)
#define APPRUN_FLAG_DATA      0x0000   //APP����Ҫ���κδ���ֱ������״̬
#define INIT_FLAG_DATA        0xFFFF   //Ĭ�ϱ�־������(��Ƭ�ӵ����)
#define UPDATE_FLAG_DATA      0xEEEE   //���ر�־������
#define UPLOAD_FLAG_DATA      0xDDDD   //�ϴ���־������
#define ERASE_FLAG_DATA       0xCCCC   //������־������

extern void    FLASH_PageErase(uint32_t PageAddress);
//////////////////////////////////////////////////////////////////////////////////////////////////////
//�û������Լ�����Ҫ����
#define STM32_FLASH_SIZE 	256 	 					//��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 	1              	//ʹ��FLASHд��(0��������;1��ʹ��)
#define FLASH_WAITETIME  	50000          	//FLASH�ȴ���ʱʱ��

//FLASH��ʼ��ַ
#define STM32_FLASH_BASE 0x08000000 		//STM32 FLASH����ʼ��ַ

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //��������  
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//��ָ����ַ��ʼд��ָ�����ȵ�����
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//��ָ����ַ��ʼ����ָ�����ȵ�����
void iap_load_boot(void);

void IAP_FLASH_WriteFlag(uint16_t flag);
uint16_t IAP_FLASH_ReadFlag(void);

								   
#endif

















