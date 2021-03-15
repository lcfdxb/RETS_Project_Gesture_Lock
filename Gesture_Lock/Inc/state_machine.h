/**
  ******************************************************************************
  * File Name          : state_machine.h
  * Description        : This file provides code for state machine in RTES 2020
	*											 spring final project, gesuture lock.
	*											 Two state machine included, one for switching working
	*											 mode, one for detecting motion.
	* @author Chengfeng Luo										 
  ******************************************************************************
  * @attention
  *	For STM32F411
  * 
  ******************************************************************************
  */
	
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
/* Exported macro ------------------------------------------------------------*/
#define SeqLength				64//max gesture sequence length
/* Exported types ------------------------------------------------------------*/
typedef struct{
	uint8_t len;
	uint8_t seq[SeqLength];
}	Gesture_Seq_t;
/* Exported constants --------------------------------------------------------*/
extern Gesture_Seq_t		g_seq;
/* Exported functions prototypes ---------------------------------------------*/
int Motion_Input_Check(void);
int State_Machine_Init(void);
int State_Update_Main(void);
