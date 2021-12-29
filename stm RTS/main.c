#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_exti.h"
#include "stdio.h"
#include "misc.h"


#define JULIAN_DATE_BASE	2440588

//========================================================================================
typedef struct
{
	uint8_t RTC_Hours;
	uint8_t RTC_Minutes;
	uint8_t RTC_Seconds;
	uint8_t RTC_Date;
} RTC_DateTimeTypeDef;

GPIO_InitTypeDef PortA;
GPIO_InitTypeDef PortB;
EXTI_InitTypeDef Exti;
NVIC_InitTypeDef Nvic;
RTC_DateTimeTypeDef RTC_DateTime;

int SysTickDelay;
uint8_t GPIO_Read = 0;
uint32_t RTC_Counter = 0;

// Get current date
unsigned char RTC_Init(void);
void Set_Bit_Pin(RTC_DateTimeTypeDef* RTC_DateTime);
void GPIOA_Init(void);
void GPIOB_Init(void);
void Exti_Init(void);
void Reset_Bit_Pin(void);
void Delay(void);
uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct);
void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct);
void Time_Change(void);
/////////////////////////////////////////////////////////////////////////




int main(void)
{
	__enable_irq();
 
  GPIOA_Init();
	GPIOB_Init();
	
	RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
	
	if (RTC_Init() == 1) {
		
		RTC_DateTime.RTC_Hours = 13;
		RTC_DateTime.RTC_Minutes = 28;
		RTC_DateTime.RTC_Seconds = 00;

		//После инициализации требуется задержка. Без нее время не устанавливается.
		RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
		Delay();
    }
	while(1)
	{        
    RTC_Counter = RTC_GetCounter();  
    Reset_Bit_Pin();
	  while (RTC_Counter == RTC_GetCounter())
       {
        Set_Bit_Pin(&RTC_DateTime);
        
       }
    }
 
}

unsigned char RTC_Init(void)
{
	// Включить тактирование модулей управления питанием и управлением резервной областью
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	// Разрешить доступ к области резервных данных
	PWR_BackupAccessCmd(ENABLE);
	// Если RTC выключен - инициализировать
	if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN)
	{
		// Сброс данных в резервной области
		RCC_BackupResetCmd(ENABLE);
		RCC_BackupResetCmd(DISABLE);

		// Установить источник тактирования кварц 32768
		RCC_LSEConfig(RCC_LSE_ON);
		while ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY) {}
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		RTC_SetPrescaler(0x7FFF); // Устанавливаем делитель, чтобы часы считали секунды

		// Включаем RTC
		RCC_RTCCLKCmd(ENABLE);

		// Ждем синхронизацию
		RTC_WaitForSynchro();

		return 1;
	}
	return 0;
}

void GPIOA_Init()		//	инициализация
{
	  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );
    PortA.GPIO_Mode         = GPIO_Mode_Out_PP;    
    PortA.GPIO_Speed        = GPIO_Speed_2MHz;     // По сути это ток который сможет обеспечить вывод
    PortA.GPIO_Pin          = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_15; // Номер ноги
	
    GPIO_Init(GPIOA, &PortA);
	  PortA.GPIO_Mode         = GPIO_Mode_IN_FLOATING;    
    PortA.GPIO_Speed        = GPIO_Speed_2MHz;     // По сути это ток который сможет обеспечить вывод
    PortA.GPIO_Pin          = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3; // Номер ноги
    GPIO_Init(GPIOA, &PortA);
	
	  Exti.EXTI_Line = EXTI_Line0|EXTI_Line1 | EXTI_Line2 | EXTI_Line3;
    Exti.EXTI_Mode = EXTI_Mode_Interrupt;
    Exti.EXTI_Trigger = EXTI_Trigger_Falling;
    Exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&Exti);
	
	  Nvic.NVIC_IRQChannel = EXTI0_IRQn | EXTI1_IRQn | EXTI2_IRQn | EXTI3_IRQn;
    Nvic.NVIC_IRQChannelPreemptionPriority = 0;
    Nvic.NVIC_IRQChannelSubPriority = 0;
    Nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&Nvic);
}
void GPIOB_Init()		//	инициализация
{
	
	  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );  
    
    PortB.GPIO_Mode         = GPIO_Mode_Out_PP;    
    PortB.GPIO_Speed        = GPIO_Speed_2MHz;     // По сути это ток который сможет обеспечить вывод
    PortB.GPIO_Pin          = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_6| GPIO_Pin_7|
	                            GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12; // Номер ноги
    GPIO_Init(GPIOB, &PortB);
}

void Reset_Bit_Pin()
{
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_2, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_4, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_RESET);
    GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);   
    GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
    GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET); 

    GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET); 
    GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);	
}

void Delay()
{
    SysTickDelay = 30;
    while (SysTickDelay!=0)
    {
    }
}


uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct) 
{
	uint16_t y;
	uint8_t m;
	uint32_t JDN;

	JDN=RTC_DateTimeStruct->RTC_Date;
	JDN+=(153*m+2)/5;
	JDN+=365*y;
	JDN+=y/4;
	JDN+=-y/100;
	JDN+=y/400;
	JDN = JDN -32045;
	JDN = JDN - JULIAN_DATE_BASE;
	JDN*=86400;
	JDN+=(RTC_DateTimeStruct->RTC_Hours*3600);
	JDN+=(RTC_DateTimeStruct->RTC_Minutes*60);
	JDN+=(RTC_DateTimeStruct->RTC_Seconds);

	return JDN;
}

void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct)
{
	unsigned long time;
	unsigned long t1, a, b, c, d, e, m;
	int mday = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	uint64_t jd = 0;;
	uint64_t jdn = 0;

	jd = ((RTC_Counter+43200)/(86400>>1)) + (2440587<<1) + 1;
	jdn = jd>>1;

	time = RTC_Counter;
	t1 = time/60;
	sec = time - t1*60;

	time = t1;
	t1 = time/60;
	min = time - t1*60;

	time = t1;
	t1 = time/24;
	hour = time - t1*24;


	a = jdn + 32044;
	b = (4*a+3)/146097;
	c = a - (146097*b)/4;
	d = (4*c+3)/1461;
	e = c - (1461*d)/4;
	m = (5*e+2)/153;
	mday = e - (153*m+2)/5 + 1;


	RTC_DateTimeStruct->RTC_Date = mday;
	RTC_DateTimeStruct->RTC_Hours = hour;
	RTC_DateTimeStruct->RTC_Minutes = min;
	RTC_DateTimeStruct->RTC_Seconds = sec;
}


void Set_Bit_Pin(RTC_DateTimeTypeDef* RTC_DateTime)
{
  
    int tens_of_second;
    tens_of_second = RTC_DateTime->RTC_Minutes/10;;
    switch(tens_of_second)
    {
         case 1:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 2:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 3:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 4:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);       
            break;
        case 5:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 6:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 7:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 8:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET); 
            break;
        case 9:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
        case 0:
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_RESET);
            break;
    } 

     tens_of_second =   RTC_DateTime->RTC_Minutes%10;
     switch(tens_of_second)
    {
                 case 1:    
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 2:
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 3:
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 4:

         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 5:
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
         Delay();
            break;
        case 6:
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 7: 
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 8:
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 9:
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
        case 0:      
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
            break;
    } 
    
    int tens_of_Minutes;
    tens_of_Minutes =  RTC_DateTime->RTC_Hours/10;
    switch(tens_of_Minutes)
    {
         case 1:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 2:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 3:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 4:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 5:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 6:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 7:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 8:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 9:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
        case 0:
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_SET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);
         Delay();
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
            break;
    } 
    tens_of_Minutes =   RTC_DateTime->RTC_Hours%10;
     switch(tens_of_Minutes)
    {
               case 1:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 2:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_9, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 3:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 4:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 5:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 6:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_11, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET); 
            break;
        case 7:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 8:
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET); 
        case 9:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
        case 0:
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET); 
         Delay();
         GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
         GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            break;
    }
     
}

void SysTick_Handler(void)
{
    if(SysTickDelay!=0)
    {
        SysTickDelay--;
    }
}

void EXTI0_IRQHandler()
{
if(EXTI_GetITStatus(EXTI_Line0)!=RESET) // Проверяем, генерируется ли прерывание
    {
			GPIO_Read = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
      if(GPIO_Read == RESET)
       {
          RTC_DateTime.RTC_Minutes += 1;
				  RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
		      Delay();
       }

    }
		EXTI_ClearFlag(EXTI_Line0);
}

void EXTI1_IRQHandler()
{
if(EXTI_GetITStatus(EXTI_Line1)!=RESET) // Проверяем, генерируется ли прерывание
    {
			GPIO_Read = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1);
      if(GPIO_Read == RESET)
       {
				 RTC_DateTime.RTC_Minutes += 10;
				 RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
		     Delay();
       }

    }
		EXTI_ClearFlag(EXTI_Line1);
}

void EXTI2_IRQHandler()
{
if(EXTI_GetITStatus(EXTI_Line2)!=RESET) // Проверяем, генерируется ли прерывание
    {
			GPIO_Read = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);
      if(GPIO_Read == RESET)
       {
          RTC_DateTime.RTC_Hours +=  1;
				  RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
		      Delay();
       }

    }
		EXTI_ClearFlag(EXTI_Line2);
}

void EXTI3_IRQHandler()
{
if(EXTI_GetITStatus(EXTI_Line3)!=RESET) // Проверяем, генерируется ли прерывание
    {
			GPIO_Read = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);
      if(GPIO_Read == RESET)
       {
          RTC_DateTime.RTC_Hours +=  10;
				  RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
		      Delay();
       }

    }
		EXTI_ClearFlag(EXTI_Line3);
}

