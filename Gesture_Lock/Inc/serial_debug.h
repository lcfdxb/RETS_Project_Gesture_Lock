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
	
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __serial_debug_H
#define __serial_debug_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#define USB_DEBUG
#ifdef SERIAL_DEBUG
#include "stm32f4xx_hal_usart.h"
#include "usart.h"
#endif
#ifdef USB_DEBUG
//For usb serial:
#include "usbd_cdc_if.h"
#include "usbd_cdc.h"
#endif

int fputc(int ch, FILE *f);
int fgetc(FILE *f);

int Plot_Data(void);
#ifdef __cplusplus
}
#endif
#endif /*__serial_debug_H */
