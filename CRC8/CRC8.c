#include "CRC8.h"
/*
说明：  CRC8校验
参数：	ptr:要校验的数组
				len:数组长度
返回值:	CRC8码
*/
uint8_t getCRC8(uint8_t *ptr,uint16_t len)
{
	uint8_t crc;
	uint8_t i;
	crc=0;
	while(len--)
	{
		crc^=*ptr++;
		for(i=0;i<8;i++)
		{
			if(crc&0x01)
				crc=(crc>>1)^0x8C;
			else 
				crc >>= 1;
		}
	}
	return crc;
}


