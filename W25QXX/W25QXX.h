#ifndef __W25Q32_H_
#define __W25Q32_H_
/**********************	 
W25QXX��������	   
**********************/ 
#include "../Common/common.h"
//typedef unsigned char uint8_t;
//typedef unsigned short int uint16_t;
//typedef unsigned int uint32_t;
//W25Qϵ��оƬ�б�	   
#define W25Q80 	0XEF13 	
#define W25Q16 	0XEF14
#define W25Q32 	0XEF15
#define W25Q64 	0XEF16
#define W25Q128	0XEF17
#define W25Q256 0XEF18

#define FLASH_SIZE					0x2000000 /* 256 MBits / 8 => 32MBytes */
#define BLOCK_SIZE					0x8000   	/* 1024 blocks of 32KBytes   */
#define SECTOR_SIZE 				0x1000    /* 8192 sectors of 4kBytes   */
#define PAGE_SIZE       		0x100     /* 131072 pages of 256 bytes */
#define MAX_SECTOR_COUNT    8192			//�����������
#define MAX_PAGE_COUNT      131072		//���ҳ��

#define FLASH_ADDR(sector,page)		(sector*SECTOR_SIZE + page*PAGE_SIZE)
#define PAGE_ADDR(m)  						(m*PAGE_SIZE)
#define SECTOR_ADDR(m)  					(m*SECTOR_SIZE)
#define BLOCK_ADDR(m) 						(m*BLOCK_SIZE)

extern uint16_t W25QXX_TYPE;					//����W25QXXоƬ�ͺ�		   

//W25QXX��Ƭѡ�ź�
#define W25Qx_Enable() 			HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)
#define W25Qx_Disable() 		HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)

////////////////////////////////////////////////////////////////////////////////// 
//ָ���
#define W25X_WriteEnable				0x06 
#define W25X_WriteDisable				0x04 
#define W25X_ReadStatusReg1			0x05 
#define W25X_ReadStatusReg2			0x35 
#define W25X_ReadStatusReg3			0x15 
#define W25X_WriteStatusReg1    0x01 
#define W25X_WriteStatusReg2    0x31 
#define W25X_WriteStatusReg3    0x11 
#define W25X_ReadData						0x03 
#define W25X_FastReadData				0x0B 
#define W25X_FastReadDual				0x3B 
#define W25X_PageProgram				0x02 
#define W25X_BlockErase					0x52 
#define W25X_SectorErase				0x20 
#define W25X_ChipErase					0xC7 
#define W25X_PowerDown					0xB9 
#define W25X_ReleasePowerDown		0xAB 
#define W25X_DeviceID						0xAB 
#define W25X_ManufactDeviceID		0x90 
#define W25X_JedecDeviceID			0x9F 
#define W25X_Enable4ByteAddr    0xB7
#define W25X_Exit4ByteAddr      0xE9
#define W25X_ResetEnable        0x66
#define W25X_ResetDevice        0x99

void W25QXX_Init(void);	//Flashʶ�𲢳�ʼ��						
void W25QXX_Reset(void);
uint16_t  W25QXX_ReadID(void);  	    		//��ȡFLASH ID
uint8_t W25QXX_ReadSR(uint8_t regno);     //��ȡ״̬�Ĵ��� 
void W25QXX_4ByteAddr_Enable(void);       //ʹ��4�ֽڵ�ַģʽ
void W25QXX_Write_SR(uint8_t regno,uint8_t sr);   //д״̬�Ĵ���
void W25QXX_Write_Enable(void);  					//дʹ�� 
void W25QXX_Write_Disable(void);					//д����
void W25QXX_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);	//д��Flash��������Ƿ��Ѳ���
void W25QXX_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead);   					//��ȡflash
void W25QXX_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);					//д��flash,�������Զ�����
void W25QXX_Erase_Chip(void);    	  	        //��Ƭ����
void W25QXX_Erase_Block(uint32_t Dst_Addr);	  //��������(32K)
void W25QXX_Erase_Sector(uint32_t Dst_Addr);	//��������(4k)
void W25QXX_Wait_Busy(void);           	      //�ȴ�����
void W25QXX_PowerDown(void);        	        //�������ģʽ
void W25QXX_WAKEUP(void);				              //����

#endif
