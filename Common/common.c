#include "common.h"
#include "stm32f1xx_hal.h"
#include "iwdg.h"
#include "../W25QXX/W25QXX.h"
#include "../CRC8/CRC8.h"

/* ���ڿɽ�������ַ����� */                                
u8 ReceiveBuff[UARTSIZE];											//DMA���ջ�����
u8 ReceiveBuff2[UARTSIZE];
volatile u8 recv_end_flag = 0, Rx_len = 0;		//���ڽ��ջ����жϱ�־,�����ַ�����
volatile u8 recv_end_flag2 = 0, Rx_len2 = 0;

u8 dataCache[UARTSIZE];						//�������ݻ�����
volatile u16 pgn_Flag = 0x0;				//PGN��¼


/* SPI��дFLASH�Ļ��������� */
//u8 wSpiData[PAGE_SIZE];
//u8 rSpiData[PAGE_SIZE];

volatile bool isStopMode = 0;			//˯��ģʽ��־λ
volatile u32 sleepDelay = 1200;		//Ϩ����ʱ����ʱ��(s)
volatile u32 sleepCounter = 0;		//��ǰϨ��ʱ��(s)
volatile u32 delayStart = 0;			//��ʱ�ȴ���ʼʱ��
volatile u32 delayCounter = 0;		//��ʱ�ȴ�ʱ��
volatile u8 delayTimes = 0;				//��ʱ�ȴ�����
volatile bool isStartUp = 1;			//���������״̬
volatile u8 SyncFlag = 0;					//����ͬ����־λ
volatile u8 StoreFlag = 0;				//����д���־λ
volatile u8 SendFlag = 0;					//�������ݱ�־λ
volatile u8 ELD_Rdy = 0;					//��ʼ����ɱ�־
volatile u8 recv_OK = 0;					//��λ��Ӧ���־
u8 ProtRecvBuff[150]={0};					//��λ��ָ���
volatile u8 ProtRecvLen = 0;			//��λ��ָ���
volatile u32 BackupAddr=0;				//���ݱ��ݵ�ַ
volatile u32 AddrIndex=0;					//����������ַ
volatile u32 SyncAddr =0;					//����ͬ����ַ
volatile u32 SyncAddrNow=0;				//����ͬ����ǰ��ַ
volatile u32 SyncAddrStart=0;			//����ͬ����ʼ��ַ
volatile u32 SyncAddrEnd=0;				//����ͬ��������ַ

void Delay_ms(int ms)
{
	int t = ms / 2000;
	int res = ms % 2000;
	for(; t>0 ;t--)
	{
		HAL_Delay(2000);
		HAL_IWDG_Refresh(&hiwdg);//��λ���Ź�
	}
	HAL_Delay(res);
}

//LED�ƹ���
void LED_Manage(void)
{
	HAL_GPIO_TogglePin(LED_ON_GPIO_Port,LED_ON_Pin);//����˸
#if 0
	//����δ����
	if(HAL_GPIO_ReadPin(BT_ON_GPIO_Port,BT_ON_Pin) == GPIO_PIN_SET)
	{
		HAL_GPIO_TogglePin(LED_ON_GPIO_Port,LED_ON_Pin);//����˸
	}
	//��������
	else if(HAL_GPIO_ReadPin(BT_ON_GPIO_Port,BT_ON_Pin) == GPIO_PIN_RESET)
	{
		HAL_GPIO_WritePin(LED_ON_GPIO_Port,LED_ON_Pin,GPIO_PIN_RESET);//�Ƴ���
	}
#endif
}

// �ַ���16����תASCII��(2λ)
char charhextoascii(char *hex_byte){
	int a = (hex_byte[0] - '0' <= 9) ? (hex_byte[0] - '0') : (hex_byte[0] - 55);
	int b = (hex_byte[1] - '0' <= 9) ? (hex_byte[1] - '0') : (hex_byte[1] - 55);
	return (char) ((a<<4) + b);
}

//16�����ַ���ת����(���ַ�)
int htoi(char x)
{
	return (int)(x - '0' <= 9) ? (x - '0') : (x - 55);
}

//��ȡ�ַ����ո���
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

//�ж��ַ��ĺϷ���
int checkASCIIRange(char s)
{
	if ((s>='0' && s<='9') || (s>='A' && s<='Z') || (s>='a' && s<='z'))
		return 1;
	else
		return 0;
}

/* ���Flash�Ƿ�Ϊ��һ��ʹ��,����ʼ����������*/
u8 Flash_Check(void)
{
	u8 tmpbuf[256] = {0};
	u8 m = 0;
	u8 check_data[16] = {0X55, 0XFA, 0X01, 0X23, 0X45, 0X67, 0X89, 0X09, 0X26, 0XEE};
	for(m=0;m<5;m++)
	{
		W25QXX_Read(tmpbuf,FLASH_ADDR(8191,0),10); //ʶ��ʹ�ñ�ʶ
		if(memcmp(tmpbuf,check_data,10) == 0)			 //FLASH�ǵ�һ��ʹ��
			break;
		else if(m >=4)														 //FLASH��һ��ʹ��
		{
			W25QXX_Erase_Sector(FLASH_ADDR(8191,0)); //��������ҳ��
			W25QXX_Write(check_data,FLASH_ADDR(8191,0),10);//д��ʹ�ñ�ʶ
			//��ʼ������ʱ������
			tmpbuf[0] = sleepDelay >> 24;
			tmpbuf[1] = sleepDelay >> 16;
			tmpbuf[2] = sleepDelay >> 8;
			tmpbuf[3] = sleepDelay;
			W25QXX_Write(tmpbuf,FLASH_ADDR(8191,1),4);
			
			//��ʼ���������ݵ�ַ
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
	//��ȡ����ʱ��
	W25QXX_Read(tmpbuf,FLASH_ADDR(8191,1),4);
	sleepDelay = (tmpbuf[0]<<24) + (tmpbuf[1]<<16) + (tmpbuf[2]<<8) + tmpbuf[3];
	
	for(m=0;m<5;m++)
	{
		//��ȡ���ݵ�ַ
		for(int i=0; i<16; i++)
		{
			W25QXX_Read(tmpbuf,FLASH_ADDR(8190,0)+i*256,256);
			for(int j=0; j<32; j++)
			{
				if(tmpbuf[j*8] == 0xAA)//�ҵ���Ч������ַ
				{
					AddrIndex = FLASH_ADDR(8190,0)+i*256+j*8;//����������ַ					
					//�����������±��ݵ�ַ
					BackupAddr = (tmpbuf[j*8+1]<<24) + (tmpbuf[j*8+2]<<16) + (tmpbuf[j*8+3]<<8) + tmpbuf[j*8+4];
					printf("[Flash_Check]2:BackupAddr=0x%08x,AddrIndex=0x%08x\r\n",BackupAddr,AddrIndex);
					return 1;
				}
			}	
		}	

		if(m>=4)
		{
			//�Ҳ�����Ч������ַ
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


