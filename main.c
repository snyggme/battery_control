#include "main.h"

uint32_t TimingDelay;
uint32_t currPEOButtonState, prevPEOButtonState;
uint8_t LowPowerMode = RESET,
		PowerOnButtonRelease = RESET;
uint8_t shutdown_now = 0;
		
const uint16_t BUTTON_PIN[BUTTONn] = {PWR_ON_BUTTON_PIN, S1_DCDC_ON_BUTTON_PIN, 
                                      PEO_ON_BUTTON_PIN}; 

/* Private function prototypes -----------------------------------------------*/
static void SYSCLKConfig_STOP(void);
void delay(uint32_t nTime);
void GPIO_Initialization(void);
void GPIO_Toggle(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint8_t getButtonState(GPIO_TypeDef* GPIOx, Button_TypeDef Button);

void SysTick_Handler(void)
{
	TimingDelay--;
	
	/* Enable PEO power only if not in STOP mode */
	if (LowPowerMode == RESET) 
	{
		currPEOButtonState = getButtonState(GPIOA, BUTTON_PEO_ON);
			
		if (currPEOButtonState != prevPEOButtonState)
		{
			/* Toggle PEO power supply */
			GPIO_Toggle(GPIOA, EN_PEO_PIN);
			
			/* Toggle PEO indication led */
			GPIO_Toggle(GPIOB, LED_PIN);
		}
				
		prevPEOButtonState = currPEOButtonState;
	}
	
	/*
	 *	this irq handler is called every 50 ms 
	 *	if duration of pressed state = 4 sec then shutdown_now = 4 / 0.05 = 80 
	 *	so if var shutdown_now == 80 --> forced power-down of the system
	 */
	if (getButtonState(GPIOA, BUTTON_PWR_ON) == RESET) 
	{
		shutdown_now++;
		
		if (shutdown_now == 80)	LowPowerMode = SET;	
	}

}

void EXTI0_1_IRQHandler(void)
{
	if(EXTI_GetITStatus(PWR_ON_BUTTON_EXTI_LINE) != RESET)
	{ 
		/* Clear the PWR_ON Button EXTI line pending bit */
		EXTI_ClearITPendingBit(PWR_ON_BUTTON_EXTI_LINE);
		
		/* check if button PWR_ON was pressed for the first time */
		if (PowerOnButtonRelease == SET) 
			LowPowerMode = SET;
		else 
			LowPowerMode = RESET;
	}
}

int main() {
	
	/* Configures all GPIO periph */
	GPIO_Initialization();
	
	/* SysTick interrupt each 50 ms */
	if (SysTick_Config(SystemCoreClock / 20))
	{ 
		/* Capture error */ 
		while (1);
	}
    
	while(1)
	{		
		/* Do that part only if power supply is not provided */
		/* This needed because of hot-plug of accumulator battery */
		if (GPIO_ReadInputDataBit(GPIOA, PS_ON_PIN) != RESET) 
		{
			/* Request to enter STOP mode with regulator in low power mode */
			PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
		
			/* Configures system clock after wake-up from STOP */
			//SYSCLKConfig_STOP();
			RCC_DeInit();
			
			/*
			**	Begin power up procedure
			*/
			
			/* Get current state of PEO_ON button */
			currPEOButtonState = prevPEOButtonState = getButtonState(GPIOA, BUTTON_PEO_ON);

			/* Enable 4S3P DC/DC: provide PSU with 12V input */
			GPIO_SetBits(GPIOA, EN_S4_DCDC_PIN);
			
			 /* Wait till PS_ON signal from system is pulled-down */  
			while(GPIO_ReadInputDataBit(GPIOA, PS_ON_PIN) != RESET);
			
			/* Send SPMC_PWR_SW signal to power on the system */
			GPIO_ResetBits(GPIOA, SPMC_PWR_SW_PIN);
			delay(10);
			GPIO_SetBits(GPIOA, SPMC_PWR_SW_PIN);
			
			/* Enable LCD power */
			GPIO_SetBits(GPIOA, EN_LCD_PIN);			
		}
		
		/* From that moment react to the PWR_ON button's clicks */
		PowerOnButtonRelease = SET;	
				
		/* 
		**	Begin power down procedure 
		*/
			
		/* Wait till the second click on the PWR_ON button to start power down procedure */
		while(LowPowerMode != SET);	
			
		/* Send SPMC_PWR_SW signal only if it is not forced power-down */
		if (shutdown_now < 80)
		{
			/* Send SPMC_PWR_SW signal to power down system */
			GPIO_ResetBits(GPIOA, SPMC_PWR_SW_PIN);
			delay(10);
			GPIO_SetBits(GPIOA, SPMC_PWR_SW_PIN);

			/* Wait till PS_ON signal from system is released */  
			while(GPIO_ReadInputDataBit(GPIOA, PS_ON_PIN) != SET);			
		}
		else 
			shutdown_now = 0;
		
		/* Disable LCD power */
		GPIO_ResetBits(GPIOA, EN_LCD_PIN);
		
		/* Forced disable of PEO power */
		GPIO_ResetBits(GPIOA, EN_PEO_PIN);
		
		/* Forced disable of PEO indication led */
		GPIO_ResetBits(GPIOB, LED_PIN);
		
		/* Disable 4S3P DC/DC: provide PSU with 12V input */
		GPIO_ResetBits(GPIOA, EN_S4_DCDC_PIN);

		PowerOnButtonRelease = RESET;	
		
		delay(10);
	}
}

uint8_t getButtonState(GPIO_TypeDef* GPIOx, Button_TypeDef Button)
{
	return (GPIOx->IDR & BUTTON_PIN[Button]) != (uint32_t)RESET ? (uint8_t)SET : (uint8_t)RESET;
}

void GPIO_Toggle(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{	
	GPIOx->ODR ^= GPIO_Pin;
}

void GPIO_Initialization()
{
	GPIO_InitTypeDef 	GPIO_IN_InitStructure,
						GPIO_OUT_InitStructure,
						GPIO_OUT_LED_InitStructure,
						GPIO_OUT_SPMC_InitStructure;
	EXTI_InitTypeDef 	EXTI_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;

	/* Enable Clocks */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);

	/* Configure Button pins as input */
	GPIO_IN_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_IN_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_IN_InitStructure.GPIO_Pin = PWR_ON_BUTTON_PIN | PEO_ON_BUTTON_PIN | S1_DCDC_ON_BUTTON_PIN | PS_ON_PIN;
	GPIO_Init(GPIOA, &GPIO_IN_InitStructure);
  
	/* Configure pins as output */
	GPIO_OUT_InitStructure.GPIO_Pin = EN_PEO_PIN | EN_LCD_PIN | EN_S4_DCDC_PIN | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_OUT_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_OUT_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_OUT_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_OUT_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_OUT_InitStructure);
	
	/* Configure LED pin as output */
	GPIO_OUT_LED_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_OUT_LED_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_OUT_LED_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_OUT_LED_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_OUT_LED_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_OUT_LED_InitStructure);
	
	/* Configure SPMC pin as output */
	GPIO_OUT_SPMC_InitStructure.GPIO_Pin = SPMC_PWR_SW_PIN;
	GPIO_OUT_SPMC_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_OUT_SPMC_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_OUT_SPMC_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_OUT_SPMC_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_OUT_SPMC_InitStructure);

    /* Connect Button EXTI Line to Button GPIO Pin */
    SYSCFG_EXTILineConfig(PWR_ON_BUTTON_EXTI_PORT_SOURCE, PWR_ON_BUTTON_EXTI_PIN_SOURCE);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = PWR_ON_BUTTON_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;    
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = PWR_ON_BUTTON_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x03;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure); 
}

static void SYSCLKConfig_STOP(void)
{  
  /* After wake-up from STOP reconfigure the system clock */
  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);
  
  /* Wait till HSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
  {}
  
  /* Enable PLL */
  RCC_PLLCmd(ENABLE);
  
  /* Wait till PLL is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
  {}
  
  /* Select PLL as system clock source */
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
  
  /* Wait till PLL is used as system clock source */
  while (RCC_GetSYSCLKSource() != 0x08)
  {}
}

void delay(uint32_t nTime) {
	TimingDelay = nTime;

	while(TimingDelay != 0);
}
