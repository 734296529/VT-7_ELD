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
#define request_cycle 3

volatile bool Vin_ed = 0; //Vin码是否已获取

//缓存数据
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
	static u8 i = 1;
	if(i)
	{
		writeCmd("AT SP A\r");//选择1939协议
		HAL_Delay(20);
		printf("autoBaudRate=%d\r\n",autoBaudRate());
		HAL_Delay(20);
		i = 0;
	}	
	writeCmd("AT SLEEP PIN 1\r");
	HAL_Delay(20);	
	writeCmd("AT SLEEP P 0\r");
	HAL_Delay(20);

	OBD_funStart();
	return 1;
}

void OBD_funStart(void)
{
	SyncFlag = 0;
	ELD_Rdy = 1;
}
void OBD_funStop(void)
{
	ELD_Rdy = 0;
}

void J1939_Handler(void)
{
	//准备发送请求
	if(pgn_Flag == 0)
	{
		sendPGN();
	}
	//pgn_Flag!=0超时没有收到应答
	else if(HAL_GetTick() - delayStart > delayCounter)
	{
		SendFlag = 1;
	}
	
	//收到168芯片应答
	if(recv_end_flag2)
	{
		recvPGN();
		recv_end_flag2 = 0;
	}
	//发送实时数据
	if(SendFlag)
	{
		sendData();
		SendFlag = 0;
		pgn_Flag = 0;
	}

}
/*
void J1939_getData(void)
{
	u32 tmp = RTC_ReadTimeCounter(&hrtc);
	static u8 i = 0;
	if(tmp - TimeStamp > 1)
	{
		TimeStamp = tmp;
		
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
		HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	}
}
*/

void sendPGN(void)
{
	static u8 i = 0;
	u32 tmp = 0;

	if(i>4)
		i = 0;

	switch(i)
	{
		case 0://Rpm转速
		{
			//获取时间戳
			tmp = RTC_ReadTimeCounter(&hrtc);
			if(tmp - TimeStamp > 1)
				TimeStamp = tmp;
			else
				return;
			//设置广播模式
			setBroadcastMode(1);
			//设置超时时间300ms
			writeCmd("at st 4b\r");
			//发送请求
			memset(&Rpm_T,0,sizeof(OBD_T));	
			writeCmd("F004\r");
			delayStart = HAL_GetTick();
			delayCounter = 310;
			pgn_Flag = 0xf004;
			i++;
		}
			break;
			
		case 1://Vss车速
		{
			//设置广播模式
			setBroadcastMode(1);			
			//设置超时时间200ms
			writeCmd("at st 32\r");
			HAL_Delay(40);
			//发送请求
			memset(&Vss_T,0,sizeof(OBD_T));	
			recv_end_flag2 = 0;
			writeCmd("FEF1\r");
			delayStart = HAL_GetTick();
			delayCounter = 210;
			pgn_Flag = 0xfef1;
			i++;
		}
			break;	

		case 2://Miles里程数
		{
			//设置广播模式
			setBroadcastMode(1);
			//设置超时时间300ms
			writeCmd("at st 4b\r");	
			//发送请求
			memset(&Miles_T,0,sizeof(OBD_T));
			memset(&TotalMiles_T,0,sizeof(OBD_T));	
			writeCmd("FEE0\r");
			delayStart = HAL_GetTick();
			delayCounter = 310;
			pgn_Flag = 0xfee0;
			i++;
		}
			break;
		
		case 3://Vin车架号
		{
			if(Vin_ed)
			{
				pgn_Flag = 0xfeec;
				SendFlag = 1;
				i++;
				break;
			}
			//设置非广播模式
			setBroadcastMode(0);
			//设置超时时间1000ms
			writeCmd("at st 4b\r");
			//发送请求
			memset(&Vin_T,0,sizeof(OBD_T));	
			writeCmd("FEEC\r");
			delayStart = HAL_GetTick();
			delayCounter = 1010;
			pgn_Flag = 0xfeec;
			i++;
		}
			break;
		
		case 4://EngineHours引擎时间
		{
			//设置非广播模式
			setBroadcastMode(0);
			//设置超时时间300ms
			writeCmd("at st 4b\r");
			//设置请求模式
			//setBroadcastMode(0);
			//发送请求
			memset(&EngineHours_T,0,sizeof(OBD_T));	
			writeCmd("FEE5\r");
			delayStart = HAL_GetTick();
			delayCounter = 310;
			pgn_Flag = 0xfee5;
			i++;
		}
			break;
		
		default:
			i = 0;
			break;
	}

}

void recvPGN(void)
{
	if(Rx_len2 < 10)
	{
		delayStart = HAL_GetTick();
		return ;
	}
	
	switch(pgn_Flag)
	{
	
		//EngineHours发动机时间 
		case 0xfee5:
			getEngineHours();
			break;
			
		//Vss车速
		case 0xfef1:
			getVss();
			break;

		//Rpm转速
		case 0xf004:
			getRpm();
			break;

		//Miles里程数
		case 0xfee0:
			getMiles();
			break;

		//VIN车架号
		case 0xfeec:
			getVin();
			break;

		default:
			break;
	}

}

void sendData(void)
{
	static u8 i = 0;
	switch(pgn_Flag)
	{	
		//Vss车速	
		case 0xfef1:
		{
			Vss_T.Data[1] = TimeStamp >> 24;
			Vss_T.Data[2] = TimeStamp >> 16;
			Vss_T.Data[3] = TimeStamp >> 8;
			Vss_T.Data[4] = TimeStamp;
			Vss_T.length = 5;
			Vss_T.type = 0X03;
		
			ELD_TtoP(Vss_T,Vss_P);
			ELD_transData(Vss_P);
		}
			break;

		//Rpm转速
		case 0xf004:
		{
			Rpm_T.Data[2] = TimeStamp >> 24;
			Rpm_T.Data[3] = TimeStamp >> 16;
			Rpm_T.Data[4] = TimeStamp >> 8;
			Rpm_T.Data[5] = TimeStamp;
			Rpm_T.length = 6;
			Rpm_T.type = 0X04;
			ELD_TtoP(Rpm_T,Rpm_P);
			ELD_transData(Rpm_P);
		}
			break;

		//Miles里程数
		case 0xfee0:
		{
			TotalMiles_T.Data[4] = Miles_T.Data[4] = TimeStamp >> 24;
			TotalMiles_T.Data[5] = Miles_T.Data[5] = TimeStamp >> 16;
			TotalMiles_T.Data[6] = Miles_T.Data[6] = TimeStamp >> 8;
			TotalMiles_T.Data[7] = Miles_T.Data[7] = TimeStamp;
			TotalMiles_T.length = Miles_T.length = 8;
			Miles_T.type = 0X05;
			TotalMiles_T.type = 0X06;
			
			ELD_TtoP(TotalMiles_T,TotalMiles_P);	
			ELD_TtoP(Miles_T,Miles_P);
			ELD_transData(Miles_P);
			HAL_Delay(40);
			ELD_transData(TotalMiles_P);
		}
			break;

		//VIN车架号
		case 0xfeec:
		{
			Vin_T.Data[17] = TimeStamp >> 24;
			Vin_T.Data[18] = TimeStamp >> 16;
			Vin_T.Data[19] = TimeStamp >> 8;
			Vin_T.Data[20] = TimeStamp;
			Vin_T.length = 21;
			Vin_T.type = 0X01;
			
			ELD_TtoP(Vin_T,Vin_P);
			ELD_transData(Vin_P);
		}
			break;
			
		//EngineHours发动机时间 
		case 0xfee5:
		{
			EngineHours_T.Data[4] = (TimeStamp >> 24);
			EngineHours_T.Data[5] = (TimeStamp >> 16);
			EngineHours_T.Data[6] = (TimeStamp >> 8);
			EngineHours_T.Data[7] = TimeStamp;
			EngineHours_T.length =	8;
			EngineHours_T.type = 0X02 ;
			
			ELD_TtoP(EngineHours_T,EngineHours_P);
			ELD_transData(EngineHours_P);
			//备份数据
			backup_Handler(i++);
			if(i>=4)
				i = 0;
		}
			break;

		default:
			break;
	}
	
}


int ELD_transData(u8* protocolData)
{
	u32 tmp = 0xFFFF;
	
	HAL_UART_Transmit(&huart3,protocolData,protocolData[2]+5,0xFFFF);//发送数据
	while(tmp--)
	{
		if(recv_OK)//上位机应答正确
		{
			recv_OK = 0;
			return 1;
		}
	}
	return 0;	
}

int ELD_TtoP(OBD_T data_t,u8* data_p)
{
	memset(data_p,0,32);
//	if(!data_t.length)//空数据
//		return -1;
	data_p[0] = 0X55;		//协议头
	data_p[1] = 0XFA;		//协议头
	data_p[2] = data_t.length+2;	//长度：参数+指令码
	data_p[3] = 0XD0;					//指令码
	data_p[4] = data_t.type;	//指令码
	memcpy(data_p+5,data_t.Data,data_t.length);		//参数
	data_p[5+data_t.length] = getCRC8(data_p+3, data_p[2]);	//CRC校验
	data_p[6+data_t.length] = 0XEE;	//协议尾
	return 1;
}

/*
	获取Vin车辆识别码(字符串类型)
*/
int getVin(void)
{
	char cat[128] = "";
	char vin[18] = "";
	u8 ret = 1;		
	
	memcpy(cat,dataCache,Rx_len2);	
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
			for(i = 0 ; i < 17; i++)
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
				//判断字符合法性
				if(!checkASCIIRange(vin[i]))
				{
					ret = 0;
					break;
				}
			}
			
		}
	}

	if(ret)
	{
		memcpy(Vin_T.Data,vin,17);
	}
	vin[17] = '\0'; 

	if(vin[0]!=vin[1] || vin[0]!=vin[2] || vin[0]!=vin[3] || vin[0]!=vin[4])
	{
		Vin_ed = 1;
	}
	SendFlag = 1;
	
	return 1;
}
/*
int getVin(void)
{
	char cat[128] = "";
	char vin[18] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Vin_T,0,sizeof(OBD_T));	
	
	//设置模式
	setBroadcastMode(0);
	writeCmd("at st fa\r");
	HAL_Delay(20);
	//发送指令
	writeCmd("feec\r");
	
	//循环超时等待接收
	while(work++ < request_cycle)
	{
		HAL_IWDG_Refresh(&hiwdg);//复位看门狗
		//收到数据
		if(readCmd((u8*)cat,&data_len,1020))
		{
			if(data_len<9)
				continue;
			else
				break;			
		}
		//没有收到数据
		else 
			break;		
	}
	writeCmd("at st 32\r");

	//数据解析
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
			for(i = 0 ; i < 17; i++)
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
				//判断字符合法性
				if(!checkASCIIRange(vin[i]))
				{
					ret = 0;
					break;
				}
			}
			
		}
	}
	
	if(ret)
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
*/

/*
	获取车辆速度
*/
int getVss(void) 
{
	char cat[64] = "";
	u32 sub = 0;	
	
	memcpy(cat,dataCache,Rx_len2);
	if(getSpaceNum(cat)>=7) //判断空格数量
	{
		u32 a = htoi(cat[6]);
		u32 b = htoi(cat[7]);
		sub = (a<<4) + b;		
	}

	if(sub && sub <=251)
	{
		Vss_T.Data[0] = sub;
	}
	SendFlag = 1;

	return sub;
}

/*
int getVss(void)
{
	char cat[64] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Vss_T,0,sizeof(OBD_T));	
	
	//设置模式
	setBroadcastMode(1);
	//发送指令
	writeCmd("FEF1\r");
	
	//循环超时等待接收
	while(work++ < request_cycle)
	{
		//收到数据
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(data_len<9)
				continue;
			else
				break;
		}
		//没有收到数据
		else 
			break;		
	}
	
	u32 sub = 0;
	
	if(getSpaceNum(cat)>=7) //判断空格数量
	{
		u32 a = htoi(cat[6]);
		u32 b = htoi(cat[7]);
		sub = (a<<4) + b;		
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
*/

/*
	获取发动机转速
*/
int getRpm(void)
{
	char cat[64] = "";
	u32 sub = 0;		

	memcpy(cat,dataCache,Rx_len2);	
	if(getSpaceNum(cat)>=7)//判断空格数量
	{
		u32 a = htoi(cat[9]);
		u32 b = htoi(cat[10]);
		u32 c = htoi(cat[12]);
		u32 d = htoi(cat[13]);
		sub = ((c<<12) + (d<<8) + (a<<4) + b) >> 3 ;
	}

	if(sub && sub <= 8032)
	{
		Rpm_T.Data[0] = sub >> 8;
		Rpm_T.Data[1] = sub ;
	}

	if(sub > 0){//转速不为0
		isStartUp = 1;		//修改发动机状态为点火
	}
	else if(sub <= 0 && !SyncFlag){//转速为0,非同步状态,且发动机状态为点火
		if(isStartUp == 1){
			sleepCounter = RTC_ReadTimeCounter(&hrtc);
		}
		isStartUp = 0;		//修改发动机状态为熄火
		Sleep_Manage();
	}
	SendFlag = 1;

	return sub;
}
/*
int getRpm(void)
{
	char cat[64] = "";
	u8 data_len = 0, work = 0;
	static u8 i = 0;
	
	memset(&Rpm_T,0,sizeof(OBD_T));	
	
	//设置模式
	setBroadcastMode(1);
	writeCmd("at st 4b\r");
	HAL_Delay(20);
	//发送指令
	writeCmd("F004\r");
	
	//循环超时等待接收
	while(work++ < request_cycle)
	{
		//收到数据
		if(readCmd((u8*)cat,&data_len,300))
		{
			if(data_len<9)
				continue;
			else
				break;
		}
		//没有收到数据
		else 
			break;		
	}
	writeCmd("at st 32\r");
	
	u32 sub = 0;	
	if(getSpaceNum(cat)>=7)//判断空格数量
	{
		u32 a = htoi(cat[9]);
		u32 b = htoi(cat[10]);
		u32 c = htoi(cat[12]);
		u32 d = htoi(cat[13]);
//		sub = (c*16 + d)*32 + (a*16 + b)/8;
		sub = ((c<<12) + (d<<8) + (a<<4) + b) >> 3 ;
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
	if(sub > 0){//转速不为0
		isStartUp = 1;		//修改发动机状态为点火
	}
	else if(sub <= 0 && !SyncFlag){//转速为0,非同步状态,且发动机状态为点火
		if(isStartUp == 1){
			sleepCounter = RTC_ReadTimeCounter(&hrtc);
		}
		isStartUp = 0;		//修改发动机状态为熄火
		Sleep_Manage();
	}
	
	return sub;
}
*/

/*
	获取发动机时间
*/
int getEngineHours(void) 
{
	char cat[64] = "";
	u32 sub = 0;

	memcpy(cat,dataCache,Rx_len2);
	if(getSpaceNum(cat)>=7)//判断空格数量
	{
		u32 a = charhextoascii(cat);
		u32 b = charhextoascii(cat+3);
		u32 c = charhextoascii(cat+6);
		u32 d = charhextoascii(cat+9);
		sub = (a +(b<<8) + (c<<16) + (d<<24)) / 20;//引擎时间
	}
	
	if(sub && sub<210554061)
	{
		EngineHours_T.Data[0] = (sub >> 24);
		EngineHours_T.Data[1] = (sub >> 16);
		EngineHours_T.Data[2] = (sub >> 8);
		EngineHours_T.Data[3] = sub;
	}
	SendFlag = 1;

	return sub;

}
/*
int getEngineHours(void) 
{
	char cat[64] = "";
	u8 data_len = 0, work = 0;
	
	memset(&EngineHours_T,0,sizeof(OBD_T));
	
	//设置模式
	setBroadcastMode(0);

	//发送指令
	writeCmd("FEE5\r");
	
	//循环超时等待接收
	while(work++ < request_cycle)
	{
		//收到数据
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(data_len<9)
				continue;
			else
				break;
		}
		//没有收到数据
		else 
			break;		
	}

	u32 sub = 0;
	if(getSpaceNum(cat)>=7)//判断空格数量
	{
		u32 a = charhextoascii(cat);
		u32 b = charhextoascii(cat+3);
		u32 c = charhextoascii(cat+6);
		u32 d = charhextoascii(cat+9);
		sub = (a +(b<<8) + (c<<16) + (d<<24)) / 20;//引擎时间
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
*/
/*
	获取里程数
*/
int getMiles(void)
{
	char cat[100] = "";
	u32 sub0 = 0,sub1 = 0;

	memcpy(cat,dataCache,Rx_len2);
	if(getSpaceNum(cat)>=7)//计算空格数量
	{
		u32 a = charhextoascii(cat);
		u32 b = charhextoascii(cat+3);
		u32 c = charhextoascii(cat+6);
		u32 d = charhextoascii(cat+9);
		u32 e = charhextoascii(cat+12);
		u32 f = charhextoascii(cat+15);
		u32 g = charhextoascii(cat+18);
		u32 h = charhextoascii(cat+21);
		sub1 = (a +(b<<8) + (c<<16) + (d<<24)) >> 3;//短里程
		sub0 = (e +(f<<8) + (g<<16) + (h<<24)) >> 3;//总里程		
	}

	if(sub1 && sub1<526385152)
	{
		Miles_T.Data[0] = (sub1 >> 24);
		Miles_T.Data[1] = (sub1 >> 16);
		Miles_T.Data[2] = (sub1 >> 8);
		Miles_T.Data[3] = sub1;
	}
	
	if(sub0 && sub0<526385152)
	{
		TotalMiles_T.Data[0] = (sub0 >> 24);
		TotalMiles_T.Data[1] = (sub0 >> 16);
		TotalMiles_T.Data[2] = (sub0 >> 8);
		TotalMiles_T.Data[3] = sub0;
	}
	SendFlag = 1;
	
	return sub1;
}
/*
int getMiles(void)
{
	char cat[100] = "";
	u8 data_len = 0, work = 0;
	
	memset(&Miles_T,0,sizeof(OBD_T));	
	memset(&TotalMiles_T,0,sizeof(OBD_T));
	
	//设置模式
	setBroadcastMode(1);
	writeCmd("at st 4b\r");
	HAL_Delay(20);	
	//发送指令
	//writeCmd("FEC1\r");//高精度
	writeCmd("FEE0\r");
	
	//循环超时等待接收
	while(work++ < request_cycle)
	{
		//收到数据
		if(readCmd((u8*)cat,&data_len,310))
		{
			if(data_len<9)
				continue;
			else		
				break;
		}
		//没有收到数据
		else 
			break;		
	}
	writeCmd("at st 32\r");

	u32 sub0 = 0,sub1 = 0;
	
	if(getSpaceNum(cat)>=7)//计算空格数量
	{
		u32 a = charhextoascii(cat);
		u32 b = charhextoascii(cat+3);
		u32 c = charhextoascii(cat+6);
		u32 d = charhextoascii(cat+9);
		u32 e = charhextoascii(cat+12);
		u32 f = charhextoascii(cat+15);
		u32 g = charhextoascii(cat+18);
		u32 h = charhextoascii(cat+21);
//		sub0 = (a +b*256 + c*65536 + d*16777216)/200;//总里程
//		sub1 = (e +f*256 + g*65536 + h*16777216)/200;//短里程
//		sub1 = (a +b*256 + c*65536 + d*16777216)/8;//短里程
//		sub0 = (e +f*256 + g*65536 + h*16777216)/8;//总里程
		sub1 = (a +(b<<8) + (c<<16) + (d<<24)) >> 3;//短里程
		sub0 = (e +(f<<8) + (g<<16) + (h<<24)) >> 3;//总里程		
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
*/
/*自适应波特率*/
int autoBaudRate(void)
{
	//设置波特率为250K
	setsetBaudRate(250);
	//测试波特率正确性
	if(baudTest())
		return 1;
	HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	
	//设置波特率为500K
	setsetBaudRate(500);
	//测试波特率正确性
	if(baudTest())
		return 2;
	HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	
	//恢复为默认250K波特率
	setsetBaudRate(250);
	HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	return 3;
}

/*设置波特率*/
int setsetBaudRate(int deep)
{
	char cat[64] = "";
	u8 work = 0, data_len = 0, ret = 0;
	
	//250K波特率
	if(deep == 250)				
	{
		writeCmd("at pp 32 off\r");
		HAL_Delay(25);
	} 
	//500K波特率
	else if(deep == 500)	
	{
		writeCmd("at pp 32 sv 01\r");
		HAL_Delay(25);
		writeCmd("at pp 32 on\r");
		HAL_Delay(25);
	} 
	//不支持其他波特率
	else
		return 0;

	work = data_len = ret = 0;
	writeCmd("at ppp\r");		//保存寄存器设置
	while(work++ < request_cycle)
	{
		//收到数据
		memset(cat,0,sizeof(cat));
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(strstr(cat,"OK"))
			{	
				ret = 1;
				break;
			}
		}
		//没有收到数据
		else 
			continue;		
	}
	HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	if(!ret){
		return ret;
	}
	
	work = data_len = ret = 0;
	writeCmd("atz\r");	//复位命令，使寄存器生效
	while(work++ < request_cycle)
	{
		//收到数据
		memset(cat,0,sizeof(cat));
		if(readCmd((u8*)cat,&data_len,400))
		{
			if(strstr(cat,"DFL168"))
			{
				ret = 1;
				break;
			}
		}
		//没有收到数据
		else 
			continue;		
	}
	HAL_IWDG_Refresh(&hiwdg);//复位看门狗
	return ret;
}	

/*测试当前波特率是否正确*/
int baudTest(void)
{
	char cat[64] ="";
	u8 work = 0, data_len = 0, ret = 0;

	//尝试发送数据
	writeCmd("FEF1\r");
//	HAL_Delay(20);	
	//循环超时等待接收
	while(work++ < request_cycle)
	{
		//收到数据
		memset(cat,0,sizeof(cat));
		if(readCmd((u8*)cat,&data_len,220))
		{
			if(strstr(cat,"not transmit"))//波特率错误
				return 0;
			else if(strstr(cat,"DATA"))		//波特率正确
			{	
				return 1;
			}
			ret = 1;	
		}
		//没有收到数据
		else 
			continue;		
	}
	return ret;
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
//	HAL_Delay(20);
	
	while(work++ < request_cycle)
	{
		//收到了数据
		if(readCmd((u8*)cat,&data_len,200))
		{
			if(data_len<9)
				return 0;
			else if(strstr(cat,"OK"))		
				return 1;
		}
		//没有收到数据
		else 
			continue;		
	}
	return 0;


}

/* 接受上层命令，协议解析 */
int protHandler(u8* recvCmd,u8 len)
{
	static int ProtState = 0;
	
	//寻找帧头
	if(recvCmd[0] == 0X55 && recvCmd[1] == 0XFA)
	{
		memset(ProtRecvBuff,0,sizeof(ProtRecvBuff));
		ProtRecvLen = 0;
		ProtState = 1;
	}
	
	memcpy(ProtRecvBuff+ProtRecvLen,recvCmd,len);
	ProtRecvLen += len;
	//寻找帧尾
	if(ProtState ==1 && recvCmd[len-1] == 0XEE)
	{
		//CRC校验
		if(getCRC8(ProtRecvBuff+3,ProtRecvBuff[2]) == ProtRecvBuff[ProtRecvLen-2]){
			ProtState = 2;//数据校验成功，可进行解析
		}
		else{
			mcuReply(0x7001,0x00);
		}
	}
		
	if(ProtState!=2)
		return 0;
	
	//进行解析
	u16 CodeNum = (ProtRecvBuff[3] << 8) + ProtRecvBuff[4]; 
	switch(CodeNum)
	{
		case 0X0001://设置同步时间
		{
			u32 timestamp = (ProtRecvBuff[5]<<24)+(ProtRecvBuff[6]<<16)+(ProtRecvBuff[7]<<8)+(ProtRecvBuff[8]);
			RTC_WriteTimeCounter(&hrtc,timestamp);
			mcuReply(0x0001,0xff);
		}
			break;
		case 0X0003://发起数据同步
		{	
			SyncFlag = 0;		
			u16 mStart=0,mEnd=0;
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
		case 0X0004://设置休眠时间
		{
			setSleepDelay(ProtRecvBuff[5]);
			isStartUp = 1;	
			mcuReply(0x0004,0xff);		
		}
			break;
		case 0X0005://设置读取数据类型
		{
		}
			break;
		case 0X5001://请求进行OTA升级
		{
			iap_load_boot();
		}			
			break;
		case 0X6001://查询单片机版本
		{
			mcuReply(0x7002,MCU_Version);
		}
			break;
		case 0X6002:
		{//查询休眠延迟时间
			mcuReply(0x7003,sleepDelay/60);
		}
			break;
		case 0X7001://上位机应答
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

/*	发送命令 */
u8 writeCmd(char *cmd)
{
	u8 ret = 0;
	
	HAL_Delay(20);
	ret = HAL_UART_Transmit(&huart2 , (u8*)cmd, strlen(cmd), 0xFFFF);

	return ret;
}

/*	获取ECU数据及相应长度 */
u8 readCmd(u8* Data,u8* data_len,u32 timeout)
{
	u32 tickstart;

	//超时接收
	tickstart = HAL_GetTick();
	while((HAL_GetTick() - tickstart) < timeout)
	{
		if(recv_end_flag2)//接收到数据
		{	
			{
				memcpy(Data, dataCache, Rx_len2); //拷贝串口接收到的数据
				*data_len = Rx_len2;               //记录数据长度
			}
			/*	接收数据长度清零	*/
			Rx_len2=0;
			return 1;	
		}			
	}
	return 0;
}

/* 数据校验应答 */
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

/* 设置熄火休眠延时 */
int setSleepDelay(u8 slpDly)
{
	if(slpDly<=0){
		return 0;
	}
	sleepDelay = slpDly*60;	 
	//计算熄火休眠延时，单位由min转换为s
	u8 tmpbuf[4] = {0};
	tmpbuf[0] = sleepDelay >> 24;
	tmpbuf[1] = sleepDelay >> 16;
	tmpbuf[2] = sleepDelay >> 8;
	tmpbuf[3] = sleepDelay;
	W25QXX_Write(tmpbuf,FLASH_ADDR(8191,1),4);	//储存休眠时间设置
	sleepCounter = RTC_ReadTimeCounter(&hrtc);
	return 1;
}

u8 backup_buf[4][32] = {0};
int backup_Handler(u8 i)
{
	//帧头
	backup_buf[i][0] = 0x55;
	backup_buf[i][1] = 0xFA;
	
	//长度
	backup_buf[i][2] = 0x15;
	
	//指令码
	backup_buf[i][3] = 0xB0;
	backup_buf[i][4] = 0x00;
	
	//参数(值+时间戳)
	/*发动机时间*/
	backup_buf[i][5] = EngineHours_T.Data[0];
	backup_buf[i][6] = EngineHours_T.Data[1];
	backup_buf[i][7] = EngineHours_T.Data[2];
	backup_buf[i][8] = EngineHours_T.Data[3];
	/*车速*/
	backup_buf[i][9] = Vss_T.Data[0];
	/*转速*/
	backup_buf[i][10] = Rpm_T.Data[0];
	backup_buf[i][11] = Rpm_T.Data[1];
	/*短里程*/
	backup_buf[i][12] = Miles_T.Data[0];
	backup_buf[i][13] = Miles_T.Data[1];
	backup_buf[i][14] = Miles_T.Data[2];
	backup_buf[i][15] = Miles_T.Data[3];
	/*总里程*/
	backup_buf[i][16] = TotalMiles_T.Data[0];
	backup_buf[i][17] = TotalMiles_T.Data[1];
	backup_buf[i][18] = TotalMiles_T.Data[2];
	backup_buf[i][19] = TotalMiles_T.Data[3];
	/*时间戳*/
	backup_buf[i][20]  = (TimeStamp >> 24);
	backup_buf[i][21]  = (TimeStamp >> 16);
	backup_buf[i][22] = (TimeStamp >> 8);
	backup_buf[i][23] = TimeStamp;
	
	//校验码
	backup_buf[i][24] = getCRC8(backup_buf[i]+3,backup_buf[i][2]);;
	
	//帧尾
	backup_buf[i][25] = 0xEE;
/*	
	//数据头
	backup_buf[i][0] = 0xB0;
	//发动机时间
	backup_buf[i][1] = EngineHours_T.Data[0];
	backup_buf[i][2] = EngineHours_T.Data[1];
	backup_buf[i][3] = EngineHours_T.Data[2];
	backup_buf[i][4] = EngineHours_T.Data[3];
	//车速
	backup_buf[i][5] = Vss_T.Data[0];
	//转速
	backup_buf[i][6] = Rpm_T.Data[0];
	backup_buf[i][7] = Rpm_T.Data[1];
	//短里程
	backup_buf[i][8] = Miles_T.Data[0];
	backup_buf[i][9] = Miles_T.Data[1];
	backup_buf[i][10] = Miles_T.Data[2];
	backup_buf[i][11] = Miles_T.Data[3];
	//总里程
	backup_buf[i][12] = TotalMiles_T.Data[0];
	backup_buf[i][13] = TotalMiles_T.Data[1];
	backup_buf[i][14] = TotalMiles_T.Data[2];
	backup_buf[i][15] = TotalMiles_T.Data[3];
	//时间戳
	backup_buf[i][16]  = (TimeStamp >> 24);
	backup_buf[i][17]  = (TimeStamp >> 16);
	backup_buf[i][18] = (TimeStamp >> 8);
	backup_buf[i][19] = TimeStamp;
	
	backup_buf[i][20] = getCRC8(backup_buf[i],20);
*/	
	if(i==3) //每隔4秒存储一次数据
	{
		StoreFlag = 1;
	}
	return 1;
}

int backup_2Flash(void)
{
	u8 write_buf[4][32] = {0};
	memcpy(write_buf,backup_buf,sizeof(write_buf));
	if(BackupAddr % SECTOR_SIZE ==0) 		//地址进入新的扇区时
	{
		W25QXX_Erase_Sector(BackupAddr);	//先擦除扇区
	}
	W25QXX_Write(write_buf[0],BackupAddr,sizeof(write_buf));	//存储4s的数据

	BackupAddr += sizeof(write_buf);		//存储地址偏移	
	if(BackupAddr >= OBD_FLASH_END)			//控制地址在限定范围内
	{
		BackupAddr = OBD_FLASH_START; 		//备份地址返回首地址
	}

	u8 tmpbuf[8] = {0};
	tmpbuf[0] = 0xAA;
	tmpbuf[1] = BackupAddr >> 24;
	tmpbuf[2] = BackupAddr >> 16;
	tmpbuf[3] = BackupAddr >> 8;
	tmpbuf[4] = BackupAddr;
	tmpbuf[5] = getCRC8(tmpbuf,5);

	if((AddrIndex+8) >= ADDR_INDEX_END)			//控制索引区在限定范围内
	{
		AddrIndex = ADDR_INDEX_START; 				//索引区地址返回首地址	
		W25QXX_Erase_Sector(FLASH_ADDR(8190,0));		//先擦除索引区
		W25QXX_Write(tmpbuf, AddrIndex, 8);	//更新索引区内容
	}	
	else
	{
		AddrIndex += 8;
		W25QXX_Write(tmpbuf, AddrIndex, 8);	//更新索引区内容
		
		tmpbuf[0] = 0x00;
		W25QXX_Write_NoCheck(tmpbuf,(AddrIndex-8), 1);	//旧索引区标记
	}
	
	
//	printf("[backup_Handler]BackupAddr=0x%08x\r\n",BackupAddr);
//	printf("[backup_Handler]AddrIndex=0x%08x\r\n\r\n",AddrIndex);
	
	StoreFlag = 0;
	return 1;
/*
	u8 tmpbuf[0] = 0x00;
	W25QXX_Write_NoCheck(tmpbuf, AddrIndex, 1);	//旧索引区标记
	
	AddrIndex += 8;											//索引区偏移
	if(AddrIndex >= ADDR_INDEX_END)			//控制索引区在限定范围内
	{
		AddrIndex = ADDR_INDEX_START; 		//索引区地址返回首地址
		W25QXX_Erase_Sector(FLASH_ADDR(8190,0));	//先擦除索引区
	}
	tmpbuf[0] = 0xAA;
	tmpbuf[1] = BackupAddr >> 24;
	tmpbuf[2] = BackupAddr >> 16;
	tmpbuf[3] = BackupAddr >> 8;
	tmpbuf[4] = BackupAddr;
	tmpbuf[5] = getCRC8(tmpbuf,5);
	W25QXX_Write_NoCheck(tmpbuf, AddrIndex, 8);	//更新索引区内容
	
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
	u16 backup_l = sizeof(backup_buf);
	u8 tmpbuf = 0;
	if(mStart<=mEnd || mStart>11520) //同步时间段合理性判断
	{
//		printf("[setSyncRange] fail 0\r\n");
		return 0;
	}
	
	SyncAddrStart = (BackupAddr >= mStart*15*backup_l) ? (BackupAddr - mStart*15*backup_l) : (FLASH_ADDR(8190,0) - (mStart*15*backup_l - BackupAddr));
	SyncAddrEnd   = BackupAddr;
	SyncAddrNow   = SyncAddrStart;
	

	W25QXX_Read(&tmpbuf, SyncAddrStart,1);
	
	if(tmpbuf == 0x55)//时间范围内都有效
	{
		SyncFlag = 1;
		return 1;
	}	
	else
	{
		u16 mid,low,high;
		low = mEnd;
		high = mStart;
//		printf("low =%d,high = %d\r\n",low,high);
		while(low<high)
		{
			if((low+1) == high)
			{
				SyncAddrStart = (BackupAddr >= high*15*backup_l) ? (BackupAddr - high*15*backup_l) : (FLASH_ADDR(8190,0) - (high*15*backup_l - BackupAddr));
				SyncAddrNow = SyncAddrStart;
				SyncFlag = 1;
				return 1;
			}
			mid=(low+high)/2;
			SyncAddrStart = (BackupAddr >= mid*15*backup_l) ? (BackupAddr - mid*15*backup_l) : (FLASH_ADDR(8190,0) - (mid*15*backup_l - BackupAddr));
			W25QXX_Read(&tmpbuf, SyncAddrStart, 1);
			if(tmpbuf == 0x55)
			{
				low = mid;
			}
			else
			{
				high = mid;
			}
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
	
	if(tmpbuf[0][0]==0x55 && tmpbuf[0][1]==0xFA)//有效的历史数据地址
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
	}
}

void sync_SendData(u8 mbuf[][32])
{
	
	u8 tmpbuf[30] = {0};
	int i = 0;
/*
	tmpbuf[0] = 0X55;
	tmpbuf[1] = 0XFA;		//协议头
	tmpbuf[2] = 0X15;		//长度21字节
	tmpbuf[3] = 0XB0;		//指令码
	tmpbuf[4] = 0x00;		//历史数据
	tmpbuf[25] = 0xEE;	//协议尾	

	for(i=0;i<4;i++)
	{				
		memcpy(tmpbuf+5,mbuf[i]+1,19);	//备份数据

		tmpbuf[24] = getCRC8(tmpbuf+3,tmpbuf[2]);//检验码

		HAL_UART_Transmit(&huart3,tmpbuf,26,0xFFFF);
		HAL_Delay(10);
	}
*/
	for(i=0;i<4;i++)
	{				
		memcpy(tmpbuf,mbuf[i],26);	//拷贝历史数据
		HAL_UART_Transmit(&huart3,tmpbuf,26,0xFFFF);
		HAL_Delay(1);
	}
}

