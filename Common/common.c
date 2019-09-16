#include "common.h"
#include "stm32f1xx_hal.h"
#include "iwdg.h"
#include "../W25QXX/W25QXX.h"
#include "../CRC8/CRC8.h"

/* 串口可接收最大字符个数 */                                
u8 ReceiveBuff[UARTSIZE];	//串口接收缓冲区
u8 ReceiveBuff2[UARTSIZE];
volatile u8 recv_end_flag = 0, Rx_len = 0;		//串口接收缓冲中断标志,接收字符长度
volatile u8 recv_end_flag2 = 0, Rx_len2 = 0;

/* SPI读写FLASH的缓冲区变量 */
//u8 wSpiData[PAGE_SIZE];
//u8 rSpiData[PAGE_SIZE];

volatile bool isStopMode = 0;			//睡眠模式标志位
volatile u32 sleepDelay = 300;		//熄火延时休眠时间(s)
volatile u32 sleepCount = 0;			//当前熄火时间(s)
volatile u8 backupSwitch = 0;			//当前存储开关
volatile bool isStartUp = 1;			//发动机点火状态
volatile u8 SyncFlag = 0;	 				//数据同步标志位
volatile u8 StoreFlag = 0;	 			//数据写入标志位
volatile u8 obd_Rdy = 0;
volatile u8 recv_OK = 0;					//上位机应答标志
u8 ProtRecvBuff[150]={0};					//上位机指令缓存
volatile u8 ProtRecvLen = 0;	  	//上位机指令长度
volatile u32 BackupAddr=0;				//数据备份地址
volatile u32 AddrIndex=0;					//备份索引地址
volatile u32 SyncAddr =0;					//数据同步地址
volatile u32 SyncAddrNow=0;				//数据同步当前地址
volatile u32 SyncAddrStart=0;			//数据同步起始地址
volatile u32 SyncAddrEnd=0;				//数据同步结束地址


void Delay_ms(int ms)
{
	HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	int t = ms / 1800;
	int res = ms % 1800;
	for(; t>0 ;t--)
	{
		HAL_Delay(1800);
		HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	}
	HAL_Delay(res);
}

//LED灯管理
void LED_Manage(void)
{
	HAL_GPIO_TogglePin(LED_ON_GPIO_Port,LED_ON_Pin);//灯闪烁
#if 0
	//蓝牙未连接
	if(HAL_GPIO_ReadPin(BT_ON_GPIO_Port,BT_ON_Pin) == GPIO_PIN_SET)
	{
		HAL_GPIO_TogglePin(LED_ON_GPIO_Port,LED_ON_Pin);//灯闪烁
	}
	//蓝牙连接
	else if(HAL_GPIO_ReadPin(BT_ON_GPIO_Port,BT_ON_Pin) == GPIO_PIN_RESET)
	{
		HAL_GPIO_WritePin(LED_ON_GPIO_Port,LED_ON_Pin,GPIO_PIN_RESET);//灯常亮
	}
#endif
}

// 字符串16进制转ASCII码(2位)
char charhextoascii(char *hex_byte){
	int a = (hex_byte[0] - '0' <= 9) ? (hex_byte[0] - '0') : (hex_byte[0] - 55);
	int b = (hex_byte[1] - '0' <= 9) ? (hex_byte[1] - '0') : (hex_byte[1] - 55);
	return (char) (16 * a + b);
}

//16进制字符串转整形(单字符)
int htoi(char x)
{
	return (int)(x - '0' <= 9) ? (x - '0') : (x - 55);
}

//获取字符串空格数
int getSpaceNum(char *str)
{
	int cnt = 0;
	for (int i = 0; i < strlen(str); i++) 
	{
		if (str[i] == ' ')
			cnt ++;
	}
	return cnt;
}

//判断字符的合法性
int checkASCIIRange(char s)
{
	if ((s>='0' && s<='9') || (s>='A' && s<='Z') || (s>='a' && s<='z'))
		return 1;
	else
		return 0;
}

/* 检查Flash是否为第一次使用,并初始化部分数据*/
u8 Flash_Check(void)
{
	u8 tmpbuf[256] = {0};
	u8 m = 0;
	u8 check_data[16] = {0X55, 0XFA, 0X01, 0X23, 0X45, 0X67, 0X89, 0X09, 0X00, 0XEE};
	for(m=0;m<5;m++)
	{
		W25QXX_Read(tmpbuf,FLASH_ADDR(8191,0),10); //识别使用标识
		if(memcmp(tmpbuf,check_data,10) == 0)			 //FLASH非第一次使用
			break;
		else if(m >=4)														 //FLASH第一次使用
		{
			W25QXX_Erase_Sector(FLASH_ADDR(8191,0)); //擦除设置页面
			W25QXX_Write(check_data,FLASH_ADDR(8191,0),10);//写入使用标识
			//初始化休眠时间设置
			tmpbuf[0] = sleepDelay >> 24;
			tmpbuf[1] = sleepDelay >> 16;
			tmpbuf[2] = sleepDelay >> 8;
			tmpbuf[3] = sleepDelay;
			W25QXX_Write(tmpbuf,FLASH_ADDR(8191,1),4);
			
			//初始化备份数据地址
			BackupAddr = OBD_FLASH_START;
			tmpbuf[0] = 0xAA;
			tmpbuf[1] = BackupAddr >> 24;
			tmpbuf[2] = BackupAddr >> 16;
			tmpbuf[3] = BackupAddr >> 8;
			tmpbuf[4] = BackupAddr;
			tmpbuf[5] = getCRC8(tmpbuf,5);
			W25QXX_Erase_Sector(FLASH_ADDR(8190,0));
			AddrIndex=FLASH_ADDR(8190,0);
			W25QXX_Write(tmpbuf, FLASH_ADDR(8190,0),6);		
			printf("[Flash_Check]1:BackupAddr=0x%08x,AddrIndex=0x%08x\r\n",BackupAddr,AddrIndex);
			return 1;
		}
	}
	//读取休眠时间
	W25QXX_Read(tmpbuf,FLASH_ADDR(8191,1),4);
	sleepDelay = (tmpbuf[0]<<24) + (tmpbuf[1]<<16) + (tmpbuf[2]<<8) + tmpbuf[3];
	
	for(m=0;m<5;m++)
	{
		//读取备份地址
		for(int i=0; i<16; i++)
		{
			W25QXX_Read(tmpbuf,FLASH_ADDR(8190,0)+i*256,256);
			for(int j=0; j<32; j++)
			{
				if(tmpbuf[j*8] == 0xAA)//找到有效索引地址
				{
					AddrIndex = FLASH_ADDR(8190,0)+i*256+j*8;//更新索引地址					
					//根据索引更新备份地址
					BackupAddr = (tmpbuf[j*8+1]<<24) + (tmpbuf[j*8+2]<<16) + (tmpbuf[j*8+3]<<8) + tmpbuf[j*8+4];
					printf("[Flash_Check]2:BackupAddr=0x%08x,AddrIndex=0x%08x\r\n",BackupAddr,AddrIndex);
					return 1;
				}
			}	
		}	

		if(m>=4)
		{
			//找不到有效索引地址
			BackupAddr = OBD_FLASH_START;
			tmpbuf[0] = 0xAA;
			tmpbuf[1] = BackupAddr >> 24;
			tmpbuf[2] = BackupAddr >> 16;
			tmpbuf[3] = BackupAddr >> 8;
			tmpbuf[4] = BackupAddr;
			tmpbuf[5] = getCRC8(tmpbuf,5);
			W25QXX_Erase_Sector(FLASH_ADDR(8190,0));
			AddrIndex=FLASH_ADDR(8190,0);
			W25QXX_Write(tmpbuf, FLASH_ADDR(8190,0),6);		
			printf("[Flash_Check]3:BackupAddr=0x%08x,AddrIndex=0x%08x\r\n",BackupAddr,AddrIndex);
			return 1;
		}
	}	

	return 1;
}


