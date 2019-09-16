#ifndef __W25Q32_H_
#define __W25Q32_H_
/**********************	 
W25QXX驱动代码	   
**********************/ 
#include "../Common/common.h"
//typedef unsigned char uint8_t;
//typedef unsigned short int uint16_t;
//typedef unsigned int uint32_t;
//W25Q系列芯片列表	   
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
#define MAX_SECTOR_COUNT    8192			//最大扇区数量
#define MAX_PAGE_COUNT      131072		//最大页数

#define FLASH_ADDR(sector,page)		(sector*SECTOR_SIZE + page*PAGE_SIZE)
#define PAGE_ADDR(m)  						(m*PAGE_SIZE)
#define SECTOR_ADDR(m)  					(m*SECTOR_SIZE)
#define BLOCK_ADDR(m) 						(m*BLOCK_SIZE)

extern uint16_t W25QXX_TYPE;					//定义W25QXX芯片型号		   

//W25QXX的片选信号
#define W25Qx_Enable() 			HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)
#define W25Qx_Disable() 		HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)

////////////////////////////////////////////////////////////////////////////////// 
//指令表
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

void W25QXX_Init(void);	//Flash识别并初始化						
void W25QXX_Reset(void);
uint16_t  W25QXX_ReadID(void);  	    		//读取FLASH ID
uint8_t W25QXX_ReadSR(uint8_t regno);     //读取状态寄存器 
void W25QXX_4ByteAddr_Enable(void);       //使能4字节地址模式
void W25QXX_Write_SR(uint8_t regno,uint8_t sr);   //写状态寄存器
void W25QXX_Write_Enable(void);  					//写使能 
void W25QXX_Write_Disable(void);					//写保护
void W25QXX_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);	//写入Flash，不检查是否已擦除
void W25QXX_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead);   					//读取flash
void W25QXX_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);					//写入flash,函数带自动擦除
void W25QXX_Erase_Chip(void);    	  	        //整片擦除
void W25QXX_Erase_Block(uint32_t Dst_Addr);	  //块区擦除(32K)
void W25QXX_Erase_Sector(uint32_t Dst_Addr);	//扇区擦除(4k)
void W25QXX_Wait_Busy(void);           	      //等待空闲
void W25QXX_PowerDown(void);        	        //进入掉电模式
void W25QXX_WAKEUP(void);				              //唤醒

#endif
