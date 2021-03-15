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
//extern USBD_HandleTypeDef hUsbDeviceFS;
/* Private variables ---------------------------------------------------------*/
uint32_t current_receive = 0;

/**
  * @brief  For redirecting printf to usb COM
  * @retval int
  */
int fputc(int ch, FILE *f)   
{
	USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t *)&ch, 1);
	while(((USBD_CDC_HandleTypeDef*)(hUsbDeviceFS.pClassData))->TxState!=0){};
  CDC_Transmit_FS((uint8_t *)&ch, 1); 
  return ch;
}


/**
  * @brief  For redirecting printf to usb COM
  * @retval int
  */
/*
int fputc(int ch, FILE *f)
{
  //HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xffff);
	CDC_Transmit_FS((uint8_t *)&ch, 1); 
  return ch;
}
*/

/**
  * @brief  For redirecting scanf to usb COM
  * @retval int
  */
int fgetc(FILE *f)
{
  uint8_t ch = 0;
  while(CDC_Received_Data_Size == 0);//wait for receive
	if(CDC_Received_Flag == 1){//first read
		CDC_Received_Flag = 0;
		current_receive = 0;
	}
	ch = CDC_Received_Data[current_receive];
	current_receive++;
	CDC_Received_Data_Size--;
	
  return ch;
}
