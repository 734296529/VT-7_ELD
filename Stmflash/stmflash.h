#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "stm32f1xx_hal.h"
#include "../Common/common.h"

/* Define the APP start address -------------------------------*/
#define ApplicationAddress    0x8005000
#define iapxaddr 0x8000000

/* IAP command------------------------------------------------ */
#define IAP_FLASH_FLAG_ADDR   (uint32_t)(ApplicationAddress - 1024 * 2)//App区域和Bootloader区域共享信息的地址(暂定大小为2K)
#define APPRUN_FLAG_DATA      0x0000   //APP不需要做任何处理，直接运行状态
#define INIT_FLAG_DATA        0xFFFF   //默认标志的数据(空片子的情况)
#define UPDATE_FLAG_DATA      0xEEEE   //下载标志的数据
#define UPLOAD_FLAG_DATA      0xDDDD   //上传标志的数据
#define ERASE_FLAG_DATA       0xCCCC   //擦除标志的数据

extern void    FLASH_PageErase(uint32_t PageAddress);
//////////////////////////////////////////////////////////////////////////////////////////////////////
//用户根据自己的需要设置
#define STM32_FLASH_SIZE 	256 	 					//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 	1              	//使能FLASH写入(0，不是能;1，使能)
#define FLASH_WAITETIME  	50000          	//FLASH等待超时时间

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 		//STM32 FLASH的起始地址

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据
void iap_load_boot(void);

void IAP_FLASH_WriteFlag(uint16_t flag);
uint16_t IAP_FLASH_ReadFlag(void);

								   
#endif

















