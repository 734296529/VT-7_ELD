#include "OBD_Fun.h"
#include "usart.h"
#include "rtc.h"
#include "iwdg.h"
#include "spi.h"
#include "../Timers/soft_timer.h"
#include "../StopMode/StopMode.h"
#include "../W25QXX/W25QXX.h"
#include "../CRC8/CRC8.h"
#include "../Stmflash/stmflash.h"

volatile bool Vin_ed = 0; //Vin���Ƿ��ѵõ
#define request_cycle 3
//��������
u32 TimeStamp = 0;
OBD_T Vin_T = {0};
OBD_T Vss_T = {0};
OBD_T Rpm_T = {0};
OBD_T EngineHours_T = {0};
OBD_T Miles_T = {0};
OBD_T TotalMiles_T = {0};

u8 Vin_P[64] = {0};
u8 Vss_P[64] = {0};
u8 Rpm_P[64] = {0};
u8 EngineHours_P[64] = {0};
u8 Miles_P[64] = {0};
u8 TotalMiles_P[64] = {0};

int DFL168_Init(void)
{
	char command[20] ="";
	strcpy(command,"AT SLEEP PIN 1\r");
	HAL_UART_Transmit(&huart2, (u8*)command, strlen(command), 0xFFFF);	
	HAL_Delay(20);
	strcpy(command,"AT SLEEP P 0\r");
	HAL_UART_Transmit(&huart2, (u8*)command, strlen(command), 0xFFFF);
	HAL_Delay(20);
	strcpy(command,"AT SP A\r");//ѡ��1939Э��
	HAL_UART_Transmit(&huart2, (u8*)command, strlen(command), 0xFFFF);	
	HAL_Delay(40);
	
	HAL_UART_Receive_DMA(&huart3,(uint8_t*)ReceiveBuff,UARTSIZE);
	HAL_Delay(200);
	OBD_funStart();
	return 1;
}

void OBD_funStart(void)
{	
  SyncFlag = 0;
	HAL_UART_Receive_DMA(&huart3,(uint8_t*)ReceiveBuff,UARTSIZE);
	TIMERS2_Start(1);
}
void OBD_funStop(void)
{
	obd_Rdy = 0;
	TIMERS2_Stop(1);
}

void OBD_Run(void)
{
	obd_Rdy = 1;
	J1939_getData();	//����2��ȡOBD����
}

void J1939_getData(void)
{
	TimeStamp = RTC_ReadTimeCounter(&hrtc);
	static u8 i = 0;

	getEngineHours();		
	OBD_transData(EngineHours_P);	
	HAL_Delay(20);	
	
	getVss();
	OBD_transData(Vss_P);	
	HAL_Delay(20);
	
	getRpm();
	OBD_transData(Rpm_P);	
	HAL_Delay(20);
	
	getMiles();
	OBD_transData(Miles_P);
	HAL_Delay(20);
	
	OBD_transData(TotalMiles_P);
	HAL_Delay(10);

	if(!Vin_ed)
		getVin();	
	OBD_transData(Vin_P);	
	HAL_Delay(20);
	
	backup_Handler(i++);
	if(i>=4){
		i = 0;
	}

}


int OBD_transData(u8* protocolData)
{
	u32 tmp = 0xFFFF;
	
	HAL_UART_Transmit(&huart3,protocolData,protocolData[2]+5,0xFFFF);//��������
	while(tmp--)
	{
		if(recv_OK)//��λ��Ӧ����ȷ
		{
			recv_OK = 0;
			return 1;
		}
	}

	return 0;	
}

int OBD_TtoP(OBD_T data_t,u8* data_p)
{
	memset(data_p,0,32);
//	if(!data_t.length)//������
//		return -1;
	data_p[0] = 0X55;		//Э��ͷ
	data_p[1] = 0XFA;		//Э��ͷ
	data_p[2] = data_t.length+2;	//���ȣ�����+ָ����
	data_p[3] = 0XD0;					//ָ����
	data_p[4] = data_t.type;	//ָ����
	memcpy(data_p+5,data_t.Data,data_t.length);		//����
	data_p[5+data_t.length] = getCRC8(data_p+3, data_p[2]);	//CRCУ��
	data_p[6+data_t.length] = 0XEE;	//Э��β
	return 1;
}
/*
//���ݱ�����Flash
int OBD_backupData(void)
{
	OBD_backupFun(Vss_P);
	OBD_backupFun(Rpm_P);
	OBD_backupFun(EngineHours_P);
	OBD_backupFun(Miles_P);	
	return 1;
}

int OBD_backupFun(u8* protocolData)
{
//	memset(rSpiData,0,255);

	W25QXX_Write(protocolData,BackupAddr,16);
	//ͬ����ַƫ��
	BackupAddr += 16;
	if(BackupAddr == OBD_FLASH_END)
	{
		BackupAddr = OBD_FLASH_START;	
	}
	if(BackupAddr%(SECTOR_SIZE*4) == 0)
	{
		OBD_refreshBackupAddr(BackupAddr);
	}
	return 1;
}
*/
int OBD_sendBackup(void)
{
	u8 tmpbuf[16] = {0};
	if(SyncAddr == OBD_FLASH_END)
		SyncAddr = OBD_FLASH_START;
	if(BackupAddr == SyncAddr)
	{
		SyncFlag = 0;
		return 1;
	}
	
	W25QXX_Read(tmpbuf,SyncAddr,16);
	if(getCRC8(tmpbuf+3,tmpbuf[2]) != tmpbuf[2]+3)//����У�����
	{
		SyncAddr += 16;		//������������
		return 1;
	}
	
	OBD_transBackup(tmpbuf,SyncAddr);	//��������
	SyncAddr += 16;

	return 1;		
}

int OBD_transBackup(u8* data,u32 addr)
{
	u8 tmpbuf[32] = {0};
	
	tmpbuf[0] = 0X55;		//Э��ͷ
	tmpbuf[1] = 0XFA;		//Э��ͷ
	tmpbuf[2] = data[2]+4;	//���ȣ�����+ָ����
	tmpbuf[3] = 0XB0;				//ָ����
	tmpbuf[4] = data[4];		//ָ����
	tmpbuf[5] = addr >> 24;	//������Flash��ַ
	tmpbuf[6] = addr >> 16;
	tmpbuf[7] = addr >> 8;
	tmpbuf[8] = addr;
	memcpy(tmpbuf+9,data+5,data[2]-2);		//����:ֵ+ʱ��
	tmpbuf[3+tmpbuf[2]] = getCRC8(tmpbuf+3, tmpbuf[2]);	//CRCУ��ֵ
	tmpbuf[4+tmpbuf[2]] = 0XEE;	//Э��β
	
//	HAL_UART_Transmit(&huart3,tmpbuf,tmpbuf[2]+5,0xFFFF);
	
	u32 tmp = 0xFFFF;
	while(tmp--)
	{
		//��λ��Ӧ����ȷ
		if(recv_OK)
		{
			recv_OK = 0;
			return 1;
		}
	}
	return 0;	
}

int OBD_refreshBackupAddr(u32 addr)
{
	u8 tmpbuf[16] = {0};
	tmpbuf[0] =addr >> 24;
	tmpbuf[1] =addr >> 16;
	tmpbuf[2] =addr >> 8;
	tmpbuf[3] =addr;
	W25QXX_Write(tmpbuf,FLASH_ADDR(1,0),16);
	return 1;
}

/*
	��ȡVin����ʶ����(�ַ�������)
*/
int getVin(void)
{
	char cat[128] = "";
	char vin[18] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Vin_T,0,sizeof(OBD_T));	
	
	//����ģʽ
	setBroadcastMode(0);
	writeCmd("at st fa\r");
	Delay_ms(20);
	//����ָ��
	writeCmd("feec\r");
	
	//ѭ����ʱ�ȴ�����
	while(work++ < request_cycle)
	{
		//�յ�����
		if(readCmd((u8*)cat,&data_len,1020))
		{
			if(data_len<9)
				continue;
			else
				break;			
		}
		//û���յ�����
		else 
			break;		
	}
	writeCmd("at st 32\r");
/*	for(u8 work = 0,ret = 0; work < request_cycle; work++)
	{
		ret = readCmd((u8*)cat,&data_len,300);
		if(ret == 2)		//�յ�ERROR����NODATA
			return 0;
		else if(ret==0)	//û���յ�����
			continue;
		else						//�յ�����
		{
			if(data_len<9)
				continue;
		}			
	}
*/
	//���ݽ���
	u8 ret = 1;		
	if (getSpaceNum(cat) >= 21) 
	{
		char* Index[3]  = {0};
		int i = 0;	
		char hex[2] = {0};
		Index[0] = strstr(cat, "01: ");
		Index[1] = strstr(cat, "02: ");
		Index[2] = strstr(cat, "03: ");
		if(Index[0] && Index[1] && Index[2])
		{
			for(i = 0, ret = 1; i < 17; i++)
			{
				if(i < 7)
				{
					hex[0] = Index[0][4+i*3];
					hex[1] = Index[0][5+i*3];
				}
				else if(i < 14)
				{
					hex[0] = Index[1][4+(i-7)*3];
					hex[1] = Index[1][5+(i-7)*3];
				}
				else
				{
					hex[0] = Index[2][4+(i-14)*3];
					hex[1] = Index[2][5+(i-14)*3];			
				}
				vin[i] = charhextoascii(hex);
				/*�ж��ַ��Ϸ���*/
				if(!checkASCIIRange(vin[i]))
				{
					ret = 0;
					break;
				}
			}
#if 0		
			for(i = 0; i < 7; i++)
			{
				hex[0] = Index[0][4+i*3];
				hex[1] = Index[0][5+i*3];
				vin[i] = charhextoascii(hex);				
			}
			for(i = 0; i < 7; i++)
			{
				hex[0] = Index[1][4+i*3];
				hex[1] = Index[1][5+i*3];
				vin[i+7] = charhextoascii(hex);
			}
			for(i = 0; i < 3; i++)
			{
				hex[0] = Index[2][4+i*3];
				hex[1] = Index[2][5+i*3];
				vin[i+14] = charhextoascii(hex);
			}
#endif
			
		}
	}
	
	if(!ret)
	{
		vin[17] = '\0';
		memcpy(Vin_T.Data,vin,17);
	}
	
	Vin_T.Data[17] = TimeStamp >> 24;
	Vin_T.Data[18] = TimeStamp >> 16;
	Vin_T.Data[19] = TimeStamp >> 8;
	Vin_T.Data[20] = TimeStamp;
	Vin_T.length = 21;
	Vin_T.type = 0X01;
	
	OBD_TtoP(Vin_T,Vin_P);
	if(vin[0]!=vin[1] || vin[0]!=vin[2] || vin[0]!=vin[3] || vin[0]!=vin[4])
	{
		Vin_ed = 1;
	}
	return 1;
}

/*
	��ȡ�����ٶ�
*/
int getVss(void) 
{
	char cat[64] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Vss_T,0,sizeof(OBD_T));	
	
	//����ģʽ
	setBroadcastMode(1);
	//����ָ��
	writeCmd("FEF1\r");
	
	//ѭ����ʱ�ȴ�����
	while(work++ < request_cycle)
	{
		//�յ�����
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(data_len<9)
				continue;
			else
				break;
		}
		//û���յ�����
		else 
			break;		
	}
	
/*	for(u8 work = 0,ret = 0; work < request_cycle; work++)
	{
		ret = readCmd((u8*)cat,&data_len,300);	
		if(ret == 2)		//�յ�"ERROR"����"NODATA"
			return 0;
		else if(ret==0)	//û���յ�����
			continue;
		else						//�յ�����
		{
			if(data_len<9)
				continue;
		}		
	}
*/	
	u32 sub = 0;
	
	if(getSpaceNum(cat)>=7) //�жϿո�����
	{
		u32 a = htoi(cat[6]);
		u32 b = htoi(cat[7]);
		sub = a * 16 + b;		
	}
	
	if(sub && sub <=251)
	{
		Vss_T.Data[0] = sub;
	}
	Vss_T.Data[1] = TimeStamp >> 24;
	Vss_T.Data[2] = TimeStamp >> 16;
	Vss_T.Data[3] = TimeStamp >> 8;
	Vss_T.Data[4] = TimeStamp;
	Vss_T.length = 5;
	Vss_T.type = 0X03;

	OBD_TtoP(Vss_T,Vss_P);

	return sub;
}

/*
	��ȡ������ת��
*/
int getRpm(void)
{
	char cat[64] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Rpm_T,0,sizeof(OBD_T));	
	
	//����ģʽ
	setBroadcastMode(1);
	writeCmd("at st 4b\r");
	Delay_ms(20);
	//����ָ��
	writeCmd("F004\r");
	
	//ѭ����ʱ�ȴ�����
	while(work++ < request_cycle)
	{
		//�յ�����
		if(readCmd((u8*)cat,&data_len,300))
		{
			if(data_len<9)
				continue;
			else
				break;
		}
		//û���յ�����
		else 
			break;		
	}
	writeCmd("at st 32\r");
	
	u32 sub = 0;	
	if(getSpaceNum(cat)>=7)//�жϿո�����
	{
		u32 a = htoi(cat[9]);
		u32 b = htoi(cat[10]);
		u32 c = htoi(cat[12]);
		u32 d = htoi(cat[13]);
		sub = (c*16 + d)*32 + (a*16 + b)/8;
	}

	if(sub && sub <= 8032)
	{
		Rpm_T.Data[0] = sub >> 8;
		Rpm_T.Data[1] = sub ;
	}
//	else{ 
//		Rpm_T.Data[0] = Rpm_P[5];
//		Rpm_T.Data[1] = Rpm_P[6];
//	}
	Rpm_T.Data[2] = TimeStamp >> 24;
	Rpm_T.Data[3] = TimeStamp >> 16;
	Rpm_T.Data[4] = TimeStamp >> 8;
	Rpm_T.Data[5] = TimeStamp;
	Rpm_T.length = 6;
	Rpm_T.type = 0X04;
	
	OBD_TtoP(Rpm_T,Rpm_P);
	if(sub > 0){//ת�ٲ�Ϊ0
		isStartUp = 1;		//�޸ķ�����״̬Ϊ���
		Sleep_Stop();			//ֹͣ���߼�ʱ
	}
	else if(sub <= 0 && !SyncFlag){//ת��Ϊ0,��ͬ��״̬,�ҷ�����״̬Ϊ���
		if(isStartUp == 1){
			Sleep_Start();
		}
		isStartUp = 0;		//�޸ķ�����״̬ΪϨ��
	}
	
	return sub;
}

/*
	��ȡ������ʱ��
*/
int getEngineHours(void) 
{
	char cat[64] = "";
	u8 data_len = 0, work = 0;
	
	memset(&EngineHours_T,0,sizeof(OBD_T));
	
	//����ģʽ
	setBroadcastMode(0);
	//����ָ��
	writeCmd("FEE5\r");
	
	//ѭ����ʱ�ȴ�����
	while(work++ < request_cycle)
	{
		//�յ�����
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(data_len<9)
				continue;
			else
				break;
		}
		//û���յ�����
		else 
			break;		
	}

	u32 sub = 0;
	if(getSpaceNum(cat)>=7)//�жϿո�����
	{
		u32 a = charhextoascii(cat);
		u32 b = charhextoascii(cat+3);
		u32 c = charhextoascii(cat+6);
		u32 d = charhextoascii(cat+9);
		sub = (a +b*256 + c*65536 + d*16777216)/20;//����ʱ��
	}
	
	if(sub && sub<210554061){
		EngineHours_T.Data[0] = (sub >> 24);
		EngineHours_T.Data[1] = (sub >> 16);
		EngineHours_T.Data[2] = (sub >> 8);
		EngineHours_T.Data[3] = sub;
	}
//	EngineHours_T.Data[0] = 0;
//	EngineHours_T.Data[1] = 0;
//	EngineHours_T.Data[2] = 0;
//	EngineHours_T.Data[3] = 0;

	EngineHours_T.Data[4] = (TimeStamp >> 24);
	EngineHours_T.Data[5] = (TimeStamp >> 16);
	EngineHours_T.Data[6] = (TimeStamp >> 8);
	EngineHours_T.Data[7] = TimeStamp;
	EngineHours_T.length =  8;
	EngineHours_T.type = 0X02 ;

	OBD_TtoP(EngineHours_T,EngineHours_P);
	
	return sub;
}

/*
	��ȡ�����
*/
int getMiles(void)
{
	char cat[100] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Miles_T,0,sizeof(OBD_T));	
	memset(&TotalMiles_T,0,sizeof(OBD_T));
	
	//����ģʽ
	setBroadcastMode(1);
	writeCmd("at st 4b\r");
	Delay_ms(20);	
	//����ָ��
	//writeCmd("FEC1\r");//�߾���
	writeCmd("FEE0\r");
	
	//ѭ����ʱ�ȴ�����
	while(work++ < request_cycle)
	{
		//�յ�����
		if(readCmd((u8*)cat,&data_len,310))
		{
			if(data_len<9)
				continue;
			else		
				break;
		}
		//û���յ�����
		else 
			break;		
	}
	writeCmd("at st 32\r");

	u32 sub0 = 0,sub1 = 0;
	
	if(getSpaceNum(cat)>=7)//����ո�����
	{
		u32 a = charhextoascii(cat);
		u32 b = charhextoascii(cat+3);
		u32 c = charhextoascii(cat+6);
		u32 d = charhextoascii(cat+9);
		u32 e = charhextoascii(cat+12);
		u32 f = charhextoascii(cat+15);
		u32 g = charhextoascii(cat+18);
		u32 h = charhextoascii(cat+21);
//		sub0 = (a +b*256 + c*65536 + d*16777216)/200;//�����
//		sub1 = (e +f*256 + g*65536 + h*16777216)/200;//�����
		sub1 = (a +b*256 + c*65536 + d*16777216)/8;//�����
		sub0 = (e +f*256 + g*65536 + h*16777216)/8;//�����
	}

	if(sub1 && sub1<526385152)
	{
		Miles_T.Data[0] = (sub1 >> 24);
		Miles_T.Data[1] = (sub1 >> 16);
		Miles_T.Data[2] = (sub1 >> 8);
		Miles_T.Data[3] = sub1;
	}
	Miles_T.Data[4] = TimeStamp >> 24;
	Miles_T.Data[5] = TimeStamp >> 16;
	Miles_T.Data[6] = TimeStamp >> 8;
	Miles_T.Data[7] = TimeStamp;
	Miles_T.length = 8;
	Miles_T.type = 0X05;
	
	if(sub0 && sub0<526385152)
	{
		TotalMiles_T.Data[0] = (sub0 >> 24);
		TotalMiles_T.Data[1] = (sub0 >> 16);
		TotalMiles_T.Data[2] = (sub0 >> 8);
		TotalMiles_T.Data[3] = sub0;
	}
	TotalMiles_T.Data[4] = TimeStamp >> 24;
	TotalMiles_T.Data[5] = TimeStamp >> 16;
	TotalMiles_T.Data[6] = TimeStamp >> 8;
	TotalMiles_T.Data[7] = TimeStamp;
	TotalMiles_T.length = 8;
	TotalMiles_T.type = 0X06;
	
	OBD_TtoP(TotalMiles_T,TotalMiles_P);	
	OBD_TtoP(Miles_T,Miles_P);

	return sub1;
}

/*���ò�����*/
int setsetBaudRate(int deep)
{
#if 0
    char *cmd1 = "at pp 32 sv 01\r";
    char *cmd2 = "at pp 32 on\r";
    char *cmd3 = "at ppp\r";
    char *cmd4 = "at pp 32 off\r";
    char *cmd5 = "atz\r";
    char cat[64];
    int work = 0;
    int ret = -1;

    if(deep == 250){
        goto tab_250;
    } else if (deep == 500){
        goto tab_500;
    } else{
        return -11;
    }

tab_250:
    Delay_ms(20);
    writeCmd("at pp 32 off\r");
    while (GetSubStrPos(cat, "at pp 32 off") < 0) 
		{
        if (work++ == request_cycle) {
            return -1;
        }
        memset(cat, 0, 64);
        ret = read_timeout(fd, 1);
        if (ret == 0) {
            read(fd, cat, sizeof(cat));
        }
        if (ret == -1) {
            pthread_mutex_unlock(&count_lock);
            return -2;
        }
    }
    goto end;

tab_500:
    Delay_ms(20);
    //First cmd1 = "at pp 32 sv 01\r"
    writeCmd("at pp 32 sv 01\r");
    while (GetSubStrPos(cat, "at pp 32 sv 01") < 0) 
		{
        if (work++ == request_cycle) {
            pthread_mutex_unlock(&count_lock);
            return -3;
        }
        memset(cat, 0, 64);
        ret = read_timeout(fd, 1);
        if (ret == 0) {
            read(fd, cat, sizeof(cat));
        }
        if (ret == -1) {
            return -4;
        }
    }

    work = 0;
    ret = -1;
    memset(cat, 0, 32);
    //Second cmd2 = "at pp 32 on\r"
    Delay_ms(20);
    writeCmd("at pp 32 on\r");
    while (GetSubStrPos(cat, "at pp 32 on") < 0) 
		{
        if (work++ == request_cycle) {
            return -5;
        }
        memset(cat, 0, 64);
        ret = read_timeout(fd, 1);
        if (ret == 0) {
            read(fd, cat, sizeof(cat));
        }
        if (ret == -1) {
            return -6;
        }
    }

end:
    work = 0;
    ret = -1;
    memset(cat, 0, 32);
    //End first cmd3 = "at ppp\r", save
    Delay_ms(20);
    writeCmd("at ppp\r");
    while (GetSubStrPos(cat, "OK") X< 0) 
		{
        if (work++ == request_cycle) {
            return -7;
        }
        memset(cat, 0, 64);
        ret = read_timeout(fd, 1);
        if (ret == 0) {
            read(fd, cat, sizeof(cat));
        }
        if (ret == -1) {
            return -8;
        }
    }

    work = 0;
    ret = -1;
    memset(cat, 0, 32);
    //End second cmd5 = "atz\r", reset
    Delay_ms(20);
    writeCmd("atz\r");
    while (GetSubStrPos(cat, "DFL168A") < 0) 
		{
        if (work++ == request_cycle) {
            return -9;
        }
        memset(cat, 0, 64);
        ret = read_timeout(fd, 1);
        if (ret == 0) {
            read(fd, cat, sizeof(cat));
        }
        if (ret == -1) {
            return -10;
        }
    }
#endif    
		return 0;
}	

int setBroadcastMode(bool mode1)
{
	u8 work = 0, data_len = 0;
	char cat[48] = "";
	char cmd[20] = "";

	if(mode1)
		sprintf(cmd, "at intrude 0\r");
	else
		sprintf(cmd, "at intrude 1\r");

	writeCmd(cmd);
	while(work++ < request_cycle)
	{
		//�յ�������
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(data_len<9)
				return 0;
			else if(strstr(cat,"OK"))		
				return 1;
		}
		//û���յ�����
		else 
			continue;		
	}
	return 0;


}

/* �����ϲ����Э����� */
int protHandler(u8* recvCmd,u8 len)
{
	static int ProtState = 0;
	
	//Ѱ��֡ͷ
	if(recvCmd[0] == 0X55 && recvCmd[1] == 0XFA)
	{
		memset(ProtRecvBuff,0,sizeof(ProtRecvBuff));
		ProtRecvLen = 0;
		ProtState = 1;
	}
	
	memcpy(ProtRecvBuff+ProtRecvLen,recvCmd,len);
	ProtRecvLen += len;
	//Ѱ��֡β
	if(ProtState ==1 && recvCmd[len-1] == 0XEE)
	{
		//CRCУ��
		if(getCRC8(ProtRecvBuff+3,ProtRecvBuff[2]) == ProtRecvBuff[ProtRecvLen-2]){
			ProtState = 2;//����У��ɹ����ɽ��н���
		}
		else{
			mcuReply(0x7001,0x00);
		}
	}
		
	if(ProtState!=2)
		return 0;
	
	//���н���
	u16 CodeNum = (ProtRecvBuff[3] << 8) + ProtRecvBuff[4]; 
	switch(CodeNum)
	{
		case 0X0001://����ͬ��ʱ��
		{
			u32 timestamp = (ProtRecvBuff[5]<<24)+(ProtRecvBuff[6]<<16)+(ProtRecvBuff[7]<<8)+(ProtRecvBuff[8]);
			RTC_WriteTimeCounter(&hrtc,timestamp);
			mcuReply(0x0001,0xff);
		}
			break;
		case 0X0003://��������ͬ��
		{	
			SyncFlag = 0;		
			u16 mStart=0,mEnd=0;
		  printf("recv cmd:");
			for(int m=0;m<10;m++)
			{
				printf("%02x",ProtRecvBuff[m]);
			}
			printf("\r\n");
			mStart = (ProtRecvBuff[5]<<8) + (ProtRecvBuff[6]); 
			mEnd   = (ProtRecvBuff[7]<<8) + (ProtRecvBuff[8]);
			
			if(!setSyncRange(mStart,mEnd))
			{
				mcuReply(0x0003,0x00);
			}
			else
			{
				mcuReply(0x0003,0xff);
			}
		}
			break;
		case 0X0004://��������ʱ��
		{
			setSleepDelay(ProtRecvBuff[5]);
			isStartUp = 1;	
			mcuReply(0x0004,0xff);		
		}
			break;
		case 0X0005://���ö�ȡ��������
		{
		}
			break;
		case 0X5001://�������OTA����
		{
			iap_load_boot();
		}			
			break;
		case 0X6001://��ѯ��Ƭ���汾
		{
			mcuReply(0x7002,MCU_Version);
		}
			break;
		case 0X6002:
		{//��ѯ�����ӳ�ʱ��
			mcuReply(0x7003,sleepDelay/60);
		}
			break;
		case 0X7001://��λ��Ӧ��
		{
			if(ProtRecvBuff[5]==0XFF)
				recv_OK = 1;
			else 
				recv_OK = 0;
		}
			break;
		
		default:
			break;
	}

	memset(ProtRecvBuff,0,sizeof(ProtRecvBuff));
	ProtRecvLen = 0;
	ProtState = 0;
	return 1;
}

/*	�������� */
u8 writeCmd(char *cmd)
{
	u8 ret = 0;

	/*  ����жϱ�־ */
	__HAL_UART_CLEAR_IDLEFLAG(&huart2);
	/* ֹͣDMA���� */
	HAL_UART_DMAStop(&huart2);
	/*	��ս��ջ�����	*/
	memset(ReceiveBuff2,0,sizeof(ReceiveBuff2));	
	/*	�������ݳ�������	*/
	Rx_len2=0;
	recv_end_flag2=0;
	/*	������һ�ν���	*/
	HAL_UART_Receive_DMA(&huart2,(u8*)ReceiveBuff2,UARTSIZE);
	
	//����ָ��
	ret = HAL_UART_Transmit(&huart2 , (u8*)cmd, strlen(cmd), 0xFFFF);
	
	return ret;
}

/*	��ȡECU���ݼ���Ӧ���� */
u8 readCmd(u8* Data,u8* data_len,u32 timeout)
{
	u32 tickstart;

	//��ʱ����
	tickstart = HAL_GetTick();
	while((HAL_GetTick() - tickstart) < timeout)
	{
		if(recv_end_flag2)//���յ�����
		{	
			{
				memcpy(Data, ReceiveBuff2, Rx_len2); //�������ڽ��յ�������
				*data_len = Rx_len2;               //��¼���ݳ���
			}
			/*	��ս��ջ�����	*/
			memset(ReceiveBuff2,0,sizeof(ReceiveBuff2));
			/*	�������ݳ�������	*/
			Rx_len2=0;
			recv_end_flag2=0;
			/*	������һ�ν���	*/
			HAL_UART_Receive_DMA(&huart2,(uint8_t*)ReceiveBuff2,UARTSIZE);
			return 1;	
		}			
	}
	return 0;
}

/* ����У��Ӧ�� */
int mcuReply(u16 cmdCode,u8 replyFlag)
{
	u8 ReplyBuff[32] = {0};
	ReplyBuff[0] = 0X55;
	ReplyBuff[1] = 0XFA;
	ReplyBuff[2] = 0X03;
	ReplyBuff[3] = (cmdCode >> 8);
	ReplyBuff[4] = cmdCode;
	ReplyBuff[5] = replyFlag;
	ReplyBuff[6] = getCRC8(ReplyBuff+3,ReplyBuff[2]);
	ReplyBuff[7] = 0XEE;
	
	HAL_UART_Transmit(&huart3,ReplyBuff,8,0xffff);
	return 1;
}

/* ����Ϩ��������ʱ */
int setSleepDelay(u8 slpDly)
{
	if(slpDly<=0){
		return 0;
	}
	sleepDelay = slpDly*60;	 
	//����Ϩ��������ʱ����λ��minת��Ϊs
	u8 tmpbuf[4] = {0};
	tmpbuf[0] = sleepDelay >> 24;
	tmpbuf[1] = sleepDelay >> 16;
	tmpbuf[2] = sleepDelay >> 8;
	tmpbuf[3] = sleepDelay;
	W25QXX_Write(tmpbuf,FLASH_ADDR(8191,1),4);	//��������ʱ������
	sleepCount = 0;
	return 1;
}

u8 backup_buf[4][32] = {0};
int backup_Handler(u8 i)
{
	//����ͷ
	backup_buf[i][0] = 0xB0;
	//������ʱ��
	backup_buf[i][1] = EngineHours_T.Data[0];
	backup_buf[i][2] = EngineHours_T.Data[1];
	backup_buf[i][3] = EngineHours_T.Data[2];
	backup_buf[i][4] = EngineHours_T.Data[3];
	//����
	backup_buf[i][5] = Vss_T.Data[0];
	//ת��
	backup_buf[i][6] = Rpm_T.Data[0];
	backup_buf[i][7] = Rpm_T.Data[1];
	//�����
	backup_buf[i][8] = Miles_T.Data[0];
	backup_buf[i][9] = Miles_T.Data[1];
	backup_buf[i][10] = Miles_T.Data[2];
	backup_buf[i][11] = Miles_T.Data[3];
	//�����
	backup_buf[i][12] = TotalMiles_T.Data[0];
	backup_buf[i][13] = TotalMiles_T.Data[1];
	backup_buf[i][14] = TotalMiles_T.Data[2];
	backup_buf[i][15] = TotalMiles_T.Data[3];
	//ʱ���
	backup_buf[i][16]  = (TimeStamp >> 24);
	backup_buf[i][17]  = (TimeStamp >> 16);
	backup_buf[i][18] = (TimeStamp >> 8);
	backup_buf[i][19] = TimeStamp;
	
	backup_buf[i][20] = getCRC8(backup_buf[i],20);
	
	if(i==3) //ÿ��4��洢һ������
	{
		StoreFlag = 1;
	}
	return 1;
}

int backup_2Flash(void)
{
	u8 write_buf[4][32] = {0};
	memcpy(write_buf,backup_buf,sizeof(write_buf));
	if(BackupAddr % SECTOR_SIZE ==0) 		//��ַ�����µ�����ʱ
	{
		W25QXX_Erase_Sector(BackupAddr);	//�Ȳ�������
	}
	W25QXX_Write_NoCheck(write_buf[0],BackupAddr,sizeof(write_buf));	//�洢4s������

	BackupAddr += sizeof(write_buf);		//�洢��ַƫ��	
	if(BackupAddr >= OBD_FLASH_END)			//���Ƶ�ַ���޶���Χ��
	{
		BackupAddr = OBD_FLASH_START; 		//���ݵ�ַ�����׵�ַ
	}

	u8 tmpbuf[8] = {0};
	tmpbuf[0] = 0xAA;
	tmpbuf[1] = BackupAddr >> 24;
	tmpbuf[2] = BackupAddr >> 16;
	tmpbuf[3] = BackupAddr >> 8;
	tmpbuf[4] = BackupAddr;
	tmpbuf[5] = getCRC8(tmpbuf,5);

	if((AddrIndex+8) >= ADDR_INDEX_END)			//�������������޶���Χ��
	{
		AddrIndex = ADDR_INDEX_START; 				//��������ַ�����׵�ַ	
		W25QXX_Erase_Sector(FLASH_ADDR(8190,0));		//�Ȳ���������
		W25QXX_Write_NoCheck(tmpbuf, AddrIndex, 8);	//��������������
	}	
	else
	{
		AddrIndex += 8;
		W25QXX_Write(tmpbuf, AddrIndex, 8);	//��������������
		
		tmpbuf[0] = 0x00;
		W25QXX_Write_NoCheck(tmpbuf,(AddrIndex-8), 1);	//�����������
	}
	
	
//	printf("[backup_Handler]BackupAddr=0x%08x\r\n",BackupAddr);
//	printf("[backup_Handler]AddrIndex=0x%08x\r\n\r\n",AddrIndex);
	
	StoreFlag = 0;
	return 1;
/*
	u8 tmpbuf[0] = 0x00;
	W25QXX_Write_NoCheck(tmpbuf, AddrIndex, 1);	//�����������
	
	AddrIndex += 8;											//������ƫ��
	if(AddrIndex >= ADDR_INDEX_END)			//�������������޶���Χ��
	{
		AddrIndex = ADDR_INDEX_START; 		//��������ַ�����׵�ַ
		W25QXX_Erase_Sector(FLASH_ADDR(8190,0));	//�Ȳ���������
	}
	tmpbuf[0] = 0xAA;
	tmpbuf[1] = BackupAddr >> 24;
	tmpbuf[2] = BackupAddr >> 16;
	tmpbuf[3] = BackupAddr >> 8;
	tmpbuf[4] = BackupAddr;
	tmpbuf[5] = getCRC8(tmpbuf,5);
	W25QXX_Write_NoCheck(tmpbuf, AddrIndex, 8);	//��������������
	
//		W25QXX_Read(tmpbuf,AddrIndex,8);
//		printf("Read 0x%08x:",AddrIndex);
//		for(int m=0;m<8;m++)
//		{
//			printf("%02x",tmpbuf[m]);
//		}
//		printf("\r\n");
	
		printf("[backup_Handler]BackupAddr=0x%08x\r\n",BackupAddr);
		printf("[backup_Handler]AddrIndex=0x%08x\r\n\r\n",AddrIndex);
	StoreFlag = 0;
	return 1;
*/
}


int setSyncRange(u16 mStart,u16 mEnd)
{
//	printf("[setSyncRange]:mStart=%d,mEnd=%d\r\n",mStart,mEnd);

	if(mStart<=mEnd || mStart>11520 || mEnd>11520) //ͬ��ʱ��κ������ж�
	{
//		printf("[setSyncRange] fail 0\r\n");
		return 0;
	}
	
	u16 backup_l = sizeof(backup_buf);
	SyncAddrStart = (BackupAddr >= mStart*backup_l) ? (BackupAddr - mStart*backup_l) : (FLASH_ADDR(8190,0) - (mStart*backup_l - BackupAddr));
	SyncAddrEnd   = (BackupAddr >= mEnd*backup_l)   ? (BackupAddr - mEnd*backup_l)   : (FLASH_ADDR(8190,0) - (mEnd*backup_l - BackupAddr));
	SyncAddrNow   = SyncAddrStart;
	
	u8 tmpbuf[2][3] = {0};
	W25QXX_Read(tmpbuf[0], SyncAddrStart,1);
	W25QXX_Read(tmpbuf[1], SyncAddrEnd, 1);
	
	if(tmpbuf[0][0]==tmpbuf[1][0] && tmpbuf[0][0] == 0xb0)//ʱ�䷶Χ�ڶ���Ч
	{
		SyncFlag = 1;
		return 1;
	}
	
	if(tmpbuf[1][0]!=0xb0 && mEnd!=0)//��ʱ�䷶Χ��������
	{
//		printf("[setSyncRange] fail 1\r\n");
		return 0;
	}
	if(tmpbuf[0][0]==0xb0)
	{
		SyncFlag = 1;
		return 1;
	}
		
	if(tmpbuf[0][0]!=0xb0)
	{
		u16 mid,low,high;
		low = mEnd;
		high = mStart;
//		printf("low =%d,high = %d\r\n",low,high);
		while(low<high)
    {
			if((low+1) == high)
			{
				SyncAddrStart = (BackupAddr >= high*backup_l) ? (BackupAddr - high*backup_l) : (FLASH_ADDR(8190,0) - (high*backup_l - BackupAddr));
				SyncAddrNow = SyncAddrStart;
				SyncFlag = 1;
				return 1;
			}
			mid=(low+high)/2;
			SyncAddrStart = (BackupAddr >= mid*backup_l) ? (BackupAddr - mid*backup_l) : (FLASH_ADDR(8190,0) - (mid*backup_l - BackupAddr));
			W25QXX_Read(tmpbuf[0], SyncAddrStart, 1);
			if(tmpbuf[0][0] == 0xb0)
			{
				low = mid;
			}
			else
			{
				high = mid;
			}
			HAL_IWDG_Refresh(&hiwdg);//��λ���Ź�
		}
	}
	
//	printf("[setSyncRange] fail 2\r\n");
	return 0;	
	
/*	
	SyncFlag = 1;
	printf("[setSyncRange]:mStart=%d,mEnd=%d\r\n",mStart,mEnd);
	printf("[setSyncRange]:SyncAddrStart=0x%08x,SyncAddrEnd=0x%08x\r\n",SyncAddrStart,SyncAddrEnd);
	return 1;
*/
}

void sync_Handler(void)
{
	u8 tmpbuf[4][32] = {0};
	
	W25QXX_Read(tmpbuf[0], SyncAddrNow, sizeof(tmpbuf));

//	printf("[sync_Handler]:0x%08x = 0x%02x\r\n",SyncAddrNow,tmpbuf[0][0]);
	
	if(tmpbuf[0][0]==0xb0)//��Ч����ʷ���ݵ�ַ
	{
		sync_SendData(tmpbuf);
	}
	
	SyncAddrNow += sizeof(tmpbuf);
	if(SyncAddrNow >= OBD_FLASH_END)
	{
		SyncAddrNow = OBD_FLASH_START;
	}
	if(SyncAddrNow == SyncAddrEnd)
	{
		mcuReply(0x0003,0xAA);
		SyncFlag = 0;
		return;
	}
}

void sync_SendData(u8 mbuf[][32])
{
	u8 tmpbuf[30] = {0};
	int i = 0;
	tmpbuf[0] = 0X55;
	tmpbuf[1] = 0XFA;		//Э��ͷ
	tmpbuf[2] = 0X15;		//����21�ֽ�
	tmpbuf[3] = 0XB0;		//ָ����
	tmpbuf[4] = 0x00;		//��ʷ����
	tmpbuf[25] = 0xEE;	//Э��β	

	for(i=0;i<4;i++)
	{				
		memcpy(tmpbuf+5,&(mbuf[i][1]),19);	//��������

		tmpbuf[24] = getCRC8(tmpbuf+3,tmpbuf[2]);//������

		HAL_UART_Transmit(&huart3,tmpbuf,26,0xFFFF);
		HAL_Delay(10);
	}
	HAL_IWDG_Refresh(&hiwdg);//���Ź���λ
}

