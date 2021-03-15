/**
  ******************************************************************************
  * File Name          : serial_debug.c
  * Description        : This file provides code for supporting printf
	*											 scanf through serial
  ******************************************************************************
  * @attention
  *	For STM32F411
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "serial_debug.h"
#include "mpu6050.h"



#ifdef SERIAL_DEBUG

/**
  * @brief  For redirecting printf to UART1
  * @retval int
  */
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xffff);
  return ch;
}

/**
  * @brief  For redirecting scanf to UART1
  * @retval int
  */
int fgetc(FILE *f)
{
  uint8_t ch = 0;
  HAL_UART_Receive(&huart1, &ch, 1, 0xffff);
  return ch;
}

#endif

/**
  * @brief  send data to Upper machine via UART1
  * @retval int
  */
extern MPU_Data_t mpu_data;
int Plot_Data(void){
	int i;
	uint8_t *p;
	uint8_t buf[17];
	int32_t x,y,z;
	buf[0] = 0x88;//start of frame
	buf[1] = 0xA1;//function
	buf[2] = 12;//len of data, bytes
	/*
	p=(uint8_t *)&mpu_data.Ax;
  buf[3]=(unsigned char)(*(p+3));
  buf[4]=(unsigned char)(*(p+2));
  buf[5]=(unsigned char)(*(p+1));
  buf[6]=(unsigned char)(*(p+0));
	
	p=(uint8_t *)&mpu_data.Ay;
  buf[7]=(unsigned char)(*(p+3));
  buf[8]=(unsigned char)(*(p+2));
  buf[9]=(unsigned char)(*(p+1));
  buf[10]=(unsigned char)(*(p+0));
	
	p=(uint8_t *)&mpu_data.Az;
  buf[11]=(unsigned char)(*(p+3));
  buf[12]=(unsigned char)(*(p+2));
  buf[13]=(unsigned char)(*(p+1));
  buf[14]=(unsigned char)(*(p+0));
	*/
	x = (int32_t)(mpu_data.Ax*1000);
	y = (int32_t)(mpu_data.Ay*1000);
	z = (int32_t)(mpu_data.Az*1000);
	
	p=(uint8_t *)&x;
  buf[3]=(unsigned char)(*(p+3));
  buf[4]=(unsigned char)(*(p+2));
  buf[5]=(unsigned char)(*(p+1));
  buf[6]=(unsigned char)(*(p+0));
	
	p=(uint8_t *)&y;
  buf[7]=(unsigned char)(*(p+3));
  buf[8]=(unsigned char)(*(p+2));
  buf[9]=(unsigned char)(*(p+1));
  buf[10]=(unsigned char)(*(p+0));
	
	p=(uint8_t *)&z;
  buf[11]=(unsigned char)(*(p+3));
  buf[12]=(unsigned char)(*(p+2));
  buf[13]=(unsigned char)(*(p+1));
  buf[14]=(unsigned char)(*(p+0));
	
	
	buf[15] = 0;//check sum
	for(i=0;i<15;i++) buf[15]+=buf[i];
	buf[16] = 0;//end of string

	HAL_UART_Transmit(&huart1, (uint8_t *)&buf, 16, 0xffff);

	return 0;
}

