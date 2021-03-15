/**
  ******************************************************************************
  * File Name          : state_machine.c
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
#include "state_machine.h"
#include "main.h"
#include "oled.h"
#include "mpu6050.h"
#include "math.h"
#include "stdio.h"

extern MPU_Data_t mpu_data;

/* Private macro -------------------------------------------------------------*/
#define Key_Pressed			HAL_GPIO_ReadPin(KEY_GPIO_Port,KEY_Pin)==0
#define ShortPressMax		1000 //ms

#define MotionGapTime   5000 //ms 
#define MotionDurTime		1000//ms
#define AccPeakGapTime	300//ms

//peak detect theshold
#define MotionPeakTH		0.50f//g

#define PeakSampNum			3	//how many samples over theshold to conform a peak
#define PeakMaxPre			0.8f	// if peaks at other axis smaller than the motion_axis_max*PeakMaxPre, motion is valid

#define MinSeqLen				3
#define FlashAddr				0x08010000//sector 4
#define FlashSector			FLASH_SECTOR_4

#define dbg 						1

/* Private typedef -----------------------------------------------------------*/
typedef enum{
	Standby = 0x00U,
	Unlock  = 0x01U,
	Record  = 0x02U
} GL_Mode;

typedef struct{
	GL_Mode 	state;
	uint32_t	updateTime;
	uint8_t		is_unlocked;
} Main_State_t;

typedef struct{
	uint8_t 	max_cnt;
	uint8_t		min_cnt;
	uint8_t		peak_cnt;	//number of peaks
	uint8_t		first_peak_dir;//0 for neg, 1 for pos
	uint32_t	peak_time;//last peak time
	float 		max_abs_val;
}	Motion_Detect_Buf_t;

typedef struct{
	uint32_t	start_time;//motion start time, update at first peak
	uint32_t	start_flag;//motion detect start
	Motion_Detect_Buf_t  x;
	Motion_Detect_Buf_t  y;
	Motion_Detect_Buf_t  z;
	
} Motion_State_t;



/* Private variables ---------------------------------------------------------*/
Main_State_t		main_state;
Motion_State_t	motion_state;
Gesture_Seq_t		g_seq;
Gesture_Seq_t		*key = (Gesture_Seq_t*)FlashAddr;
/* Private function prototypes -----------------------------------------------*/
void Standby_Print(Main_State_t* s);
int Motion_Detect_Buf_Init(Motion_Detect_Buf_t* mdb);
int Motion_State_Init(Motion_State_t* ms);
int Main_State_Init(Main_State_t* s);
int Gesture_Seq_Init(Gesture_Seq_t* g);
int Flash_Init(void);
int Motion_Seq_Print(void);
int Motion_Seq_Save(void);
int Motion_Seq_Check(void);
/* Private user code ---------------------------------------------------------*/

int State_Machine_Init(void){
	uint8_t i;
	Main_State_Init(&main_state);
	Motion_State_Init(&motion_state);
	Gesture_Seq_Init(&g_seq);
	if(key->len >= SeqLength){
		Flash_Init();
	}
	printf("Key is: ");
	for(i=0;i<key->len;i++){
		printf("%d ",key->seq[i]);
	}
	printf("\r\n");
	return 0;
}

/**
  * @brief  main function state initialize
	*	@param	state variable
  * @retval int
  */
int Main_State_Init(Main_State_t* s){
	s->state = Standby;
	s->updateTime = HAL_GetTick();
	s->is_unlocked = 0;
	Standby_Print(s);
	return 0;
}

/**
  * @brief  Motion_Detect_Buf initialize
	*	@param	state variable
  * @retval int
  */
int Motion_Detect_Buf_Init(Motion_Detect_Buf_t* mdb){
	mdb->max_cnt   =0;
	mdb->min_cnt   =0;
	mdb->peak_cnt  =0;
	mdb->peak_time =HAL_GetTick();
	mdb->max_abs_val = 0;
	return 0;
}

/**
  * @brief  Motion_State initialize
	*	@param	state variable
  * @retval int
  */
int Motion_State_Init(Motion_State_t* ms){
	ms->start_time = HAL_GetTick();
	ms->start_flag = 0;
	Motion_Detect_Buf_Init(&ms->x);
	Motion_Detect_Buf_Init(&ms->y);
	Motion_Detect_Buf_Init(&ms->z);
	return 0;
}

/**
  * @brief  Gesture sequence initialize
	*	@param	Gesture sequence variable
  * @retval int
  */
int Gesture_Seq_Init(Gesture_Seq_t* g){
	g->len = 0;
	return 0;
}

int Flash_Save_Seq(Gesture_Seq_t* k){
	int i;
	uint8_t *p = (uint8_t*)k;
	
	HAL_FLASH_Unlock();
	FLASH_Erase_Sector(FlashSector, FLASH_VOLTAGE_RANGE_3);
	FLASH_WaitForLastOperation(1000);
	for(i=0;i<k->len+1;i++){
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, FlashAddr+i, *p);
		p++;
		FLASH_WaitForLastOperation(1000);
	}
	HAL_FLASH_Lock();
	return 0;
}

/**
  * @brief  initialize key seq in flash
  * @retval int
  */
int Flash_Init(void){
	Gesture_Seq_t k;
	k.len = 4;
	k.seq[0] = 12;
	k.seq[1] = 11;
	k.seq[2] = 10;
	k.seq[3] = 9;
	Flash_Save_Seq(&k);
	return 0;
}

/**
  * @brief  Show screen message in standby state
	*	@param	state variable
  * @retval int
  */
void Standby_Print(Main_State_t* s){
	OLED_Clear();
	if(s->is_unlocked == 0){
		OLED_ShowString(0,0,"Locked!");
		OLED_ShowString(0,2,"Press to Unlock.");
	}
	else{
		OLED_ShowString(0,0,"Unlocked!");
		OLED_ShowString(0,2,"Long Press to");
		OLED_ShowString(0,4,"Record");
	}
}

/**
  * @brief  main function state update
  * @retval int
  */
int State_Update_Main(void){
	Main_State_t* s = &main_state;
	/************Stand by state*********************/
	if(s->state == Standby){
		if(Key_Pressed){
			HAL_Delay(20);//debouncer
			if(Key_Pressed){
				HAL_Delay(ShortPressMax);
				if(Key_Pressed){			//long press
					while(Key_Pressed); //wait till release
					if(s->is_unlocked){ //allow to get in record mode
						OLED_Clear();
						OLED_ShowString(0,0,"Record Mode");
						Motion_State_Init(&motion_state);//init motion state variable
						Gesture_Seq_Init(&g_seq);
						s->state = Record;
						s->updateTime = HAL_GetTick();
						return 0;
					}
					else{								//show error message
						OLED_Clear();
						OLED_ShowString(0,0,"Unlock first!");
						HAL_Delay(1000);
						Standby_Print(s);
						return 0;
					}
				}
				else{								//short press
					OLED_Clear();
					OLED_ShowString(0,0,"Unlock Mode");
					Motion_State_Init(&motion_state);//init motion state variable
					Gesture_Seq_Init(&g_seq);
					s->state = Unlock;
					s->updateTime = HAL_GetTick();
					return 0;
				}
			}
		}
	}
	/************Unlock state*********************/
	else if(s->state == Unlock){
		if(HAL_GetTick() - s->updateTime > MotionGapTime){		//end of sequence
			OLED_Clear();
			OLED_ShowString(0,0,"Checking...");
			HAL_Delay(300);																		//delay to make user feels better
			if(Motion_Seq_Check()){														//match
				OLED_ShowString(0,2,"Match!");
				HAL_Delay(300);																		//delay to make user feels better
				OLED_ShowString(0,4,"Unlock!!!!");
				HAL_Delay(3000);																		//delay to make user feels better
				s->is_unlocked = 1;
				s->state = Standby;
				s->updateTime = HAL_GetTick();
				Standby_Print(s);
				return 0;
			}
			else{
				OLED_ShowString(0,2,"Fail!!!!");
				HAL_Delay(3000);																		//delay to make user feels better
				s->is_unlocked = 0;
				s->state = Standby;
				s->updateTime = HAL_GetTick();
				Standby_Print(s);
				return 0;
			}
		}
		else{																								//wait for new input
			if(Motion_Input_Check()){
				if(dbg == 1)printf("Gesture Num: %d, seq len: %d, pitch: %5.2f\r\n",g_seq.seq[g_seq.len-1],g_seq.len,mpu_data.pitch);
				OLED_Clear();
				OLED_ShowString(0,0,"Unlock Mode");
				OLED_ShowString(0,2,"Last Ges:");
				OLED_ShowNum(80,2,g_seq.seq[g_seq.len-1],2,16);
				OLED_ShowString(0,4,"Ges Len:");
				OLED_ShowNum(80,4,g_seq.len,2,16);
				s->updateTime = HAL_GetTick();
				HAL_Delay(200); //avoid motion overlap
			}
			return 0;
		}
		
	}
	/************Record state*********************/
	else if(s->state == Record){
		if(HAL_GetTick()-s->updateTime > MotionGapTime){		//end of sequence
			if(g_seq.len >= MinSeqLen){
				OLED_Clear();
				OLED_ShowString(0,0,"Saving...");
				HAL_Delay(300);																		//delay to make user feels better
				Motion_Seq_Save();
				Motion_Seq_Print();
				OLED_ShowString(0,6,"Saved!");
				HAL_Delay(6000);																	//delay to make user feels better
				s->is_unlocked = 0;
			}
			else{
				OLED_Clear();
				OLED_ShowString(0,0,"Too short!!!");
				HAL_Delay(2000);		
			}
			s->state = Standby;
			s->updateTime = HAL_GetTick();
			Standby_Print(s);
			return 0;
		}
		else{																								//wait for new input
			if(Motion_Input_Check()){
				if(dbg == 1)printf("Gesture Num: %d, seq len: %d, pitch: %5.2f\r\n",g_seq.seq[g_seq.len-1],g_seq.len,mpu_data.pitch);
				OLED_Clear();
				OLED_ShowString(0,0,"Record Mode");
				OLED_ShowString(0,2,"Last Ges:");
				OLED_ShowNum(80,2,g_seq.seq[g_seq.len-1],2,16);
				OLED_ShowString(0,4,"Ges Len:");
				OLED_ShowNum(80,4,g_seq.len,2,16);
				s->updateTime = HAL_GetTick();
				HAL_Delay(200); //avoid motion overlap
			}
			return 0;
		}
	}
	else return -1;//error
	return -1;
}


/**
  * @brief  Peak detecter for an axis, count to 3 peaks to
	*					conform a gesture. 
	*	@param	mdb		address of motion detect buffer
	*	@param	acc		input accelerate
	* @retval int
  */
int Motion_Peak_Update(Motion_Detect_Buf_t *mdb, float acc){
	if(mdb->peak_cnt == 3){		//a gesture just detected
		Motion_Detect_Buf_Init(mdb);//reset buffer
	}
	if(mdb->peak_cnt == 0){				//wait for first peak
		if(acc > MotionPeakTH){
			mdb->max_cnt++;
		}
		else if(acc < -MotionPeakTH){
			mdb->min_cnt++;
		}
		else{
			mdb->max_cnt = 0;
			mdb->min_cnt = 0;
		}
		if(mdb->max_cnt==PeakSampNum){ //first pos peak get
			mdb->max_cnt = 0;
			mdb->first_peak_dir = 1;
			mdb->peak_cnt = 1;
			mdb->peak_time = HAL_GetTick();
		}
		else if(mdb->min_cnt==PeakSampNum){ //first pos peak get
			mdb->min_cnt = 0;
			mdb->first_peak_dir = 0;
			mdb->peak_cnt = 1;
			mdb->peak_time = HAL_GetTick();
		}
	}
	else if(mdb->peak_cnt == 1){	//wait for second peak
		if(HAL_GetTick() - mdb->peak_time > AccPeakGapTime){	//time out
			Motion_Detect_Buf_Init(mdb);//reset buffer
			return 0;
		}
		if(mdb->first_peak_dir == 1){	//last peak is pos, wait for neg
			if(acc < -MotionPeakTH){
				mdb->min_cnt++;
			}
			else{
				mdb->min_cnt = 0;
			}
		}
		else{													//last peak is neg, wait for pos
			if(acc > MotionPeakTH){
				mdb->max_cnt++;
			}
			else{
				mdb->max_cnt = 0;
			}
		}
		//check peak update
		if(mdb->max_cnt == PeakSampNum || mdb->min_cnt == PeakSampNum){
			mdb->max_cnt = 0;
			mdb->min_cnt = 0;
			mdb->peak_cnt = 2;
			mdb->peak_time = HAL_GetTick();
		}
	}
	else if(mdb->peak_cnt == 2){		//wait for the last peak
		if(HAL_GetTick() - mdb->peak_time > AccPeakGapTime){	//time out
			Motion_Detect_Buf_Init(mdb);//reset buffer
			return 0;
		}
		if(mdb->first_peak_dir == 1){	//first peak is pos, wait for pos
			if(acc > MotionPeakTH){
				mdb->max_cnt++;
			}
			else{
				mdb->max_cnt = 0;
			}
		}
		else{													//first peak is neg, wait for neg
			if(acc < -MotionPeakTH){
				mdb->min_cnt++;
			}
			else{
				mdb->min_cnt = 0;
			}
		}
		//check peak update
		if(mdb->max_cnt == PeakSampNum || mdb->min_cnt == PeakSampNum){
			mdb->max_cnt = 0;
			mdb->min_cnt = 0;
			mdb->peak_cnt = 3;
			mdb->peak_time = HAL_GetTick();
		}
	}
	//update max
	if(mdb->max_abs_val < fabs(acc)) mdb->max_abs_val=fabs(acc);
	return 0;
}


/**
  * @brief  
	*	@param	
	* @retval int 
	*       	0 : Palm up
	*					6 : Palm left
	*					12: Palm down
  */
int Motion_Roll_Check(void){
	if(mpu_data.pitch < -45) return 0;
	else if(mpu_data.pitch < 45) return 6;
	else return 12;
}

/**
  * @brief  Check if a valid motion is made
	* @retval int 
	*       	0: no new gesture
	*					1: new gesture done
  */
int Motion_Input_Check(void){
	if(motion_state.start_flag == 1 && HAL_GetTick()-motion_state.start_time > MotionDurTime){ //time out
		Motion_State_Init(&motion_state);
		if(dbg == 1)printf("motion time out!\r\n");
		return 0;
	}
	
	if(mpu_data.UpdateFlag){	//new data coming
		mpu_data.UpdateFlag = 0; //reset flag
		Motion_Peak_Update(&motion_state.x, mpu_data.Ax);
		Motion_Peak_Update(&motion_state.y, mpu_data.Ay);
		Motion_Peak_Update(&motion_state.z, mpu_data.Az);
		//check if motion start
		if(motion_state.start_flag == 0){	
			if(		motion_state.x.peak_cnt == 1 || 
						motion_state.y.peak_cnt == 1 ||
						motion_state.z.peak_cnt == 1){
				motion_state.start_flag = 1;
				motion_state.start_time = HAL_GetTick();
				if(dbg == 1)printf("motion start!\r\n");
			}
		}
		//check if a gesture completed
		if(motion_state.x.peak_cnt == 3){
			if(motion_state.x.max_abs_val*PeakMaxPre>motion_state.y.max_abs_val && 
				motion_state.x.max_abs_val*PeakMaxPre>motion_state.z.max_abs_val){
				g_seq.seq[g_seq.len] = (1+motion_state.x.first_peak_dir)+Motion_Roll_Check();
				g_seq.len++;
				Motion_State_Init(&motion_state);
				if(dbg == 1)printf("	motion at x %d!\r\n",motion_state.x.first_peak_dir);
				return 1;
			}
			if(dbg == 1)printf("	peak rej x!\r\n");
		}
		else if(motion_state.y.peak_cnt == 3){
			if(motion_state.y.max_abs_val*PeakMaxPre>motion_state.x.max_abs_val && 
				motion_state.y.max_abs_val*PeakMaxPre>motion_state.z.max_abs_val){
				g_seq.seq[g_seq.len] = (3+motion_state.y.first_peak_dir)+Motion_Roll_Check();
				g_seq.len++;
				Motion_State_Init(&motion_state);
				if(dbg == 1)printf("	motion at y %d!\r\n",motion_state.y.first_peak_dir);
				return 1;
			}
			if(dbg == 1)printf("	peak rej y!\r\n");
		}
		else if(motion_state.z.peak_cnt == 3){
			if(motion_state.z.max_abs_val*PeakMaxPre>motion_state.x.max_abs_val && 
				motion_state.z.max_abs_val*PeakMaxPre>motion_state.y.max_abs_val){
				g_seq.seq[g_seq.len] = (5+motion_state.z.first_peak_dir)+Motion_Roll_Check();
				g_seq.len++;
				Motion_State_Init(&motion_state);
				if(dbg == 1)printf("	motion at z %d!\r\n",motion_state.z.first_peak_dir);
				return 1;
			}
			if(dbg == 1)printf("	peak rej z!\r\n");
		}
		return 0;
	}
	else return 0;
}

/**
  * @brief  Check if a sequence is right
	* @retval int 
	*       	0: wrong
	*					1: right
  */
int Motion_Seq_Check(void){
	uint8_t i;
	if(key->len != g_seq.len){
		Gesture_Seq_Init(&g_seq);
		return 0;
	}
	for(i=0;i<g_seq.len;i++){
		if(key->seq[i] != g_seq.seq[i]){
			Gesture_Seq_Init(&g_seq);
			return 0;
		}
	}
	Gesture_Seq_Init(&g_seq);
	return 1;
}

/**
  * @brief  Save sequence to flash
	* @retval int 
  */
int Motion_Seq_Save(void){
	int i;
	Flash_Save_Seq(&g_seq);
	printf("New key is: ");
	for(i=0;i<key->len;i++){
		printf("%d ",key->seq[i]);
	}
	return 0;
}

int Motion_Seq_Print(void){
	char buf[64];
	char *p= &buf[0];
	int i;
	sprintf(buf,"New:");
	p += 4;
	for(i=0;i<key->len;i++){
		sprintf(p,"%2d,",key->seq[i]);
		p +=3;
	}
	*p = 0;
	OLED_ShowString(0,2,&buf[0]);
	return 0;
}
