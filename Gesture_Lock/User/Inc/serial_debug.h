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
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_usart.h"
//For usb serial:
#include "usbd_cdc_if.h"
#include "usb_device.h"

int fputc(int ch, FILE *f);
int fgetc(FILE *f);

#ifdef __cplusplus
}
#endif
#endif /*__serial_debug_H */
