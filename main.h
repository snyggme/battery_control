#include "stm32f0xx.h"

typedef enum 
{
	BUTTON_PWR_ON = 0,
	BUTTON_PEO_ON = 1,
	BUTTON_S1_DCDC_ON = 2,
} Button_TypeDef;

/**
 * @brief Power supply ON signal
 */
#define PS_ON_PIN                     		GPIO_Pin_7
/**
 * @brief Supply power managment controller POWER signal
 */
#define SPMC_PWR_SW_PIN                     GPIO_Pin_6
/**
 * @brief Enable transparent heating element (Prozrachniy Element Obogreva)
 */
#define EN_PEO_PIN                     		GPIO_Pin_1
/**
 * @brief Enable LCD
 */
#define EN_LCD_PIN                     		GPIO_Pin_3
/**
 * @brief Enable 4S3P accumulator battery DCDC converter
 */
#define EN_S4_DCDC_PIN                     	GPIO_Pin_4
/**
 * @brief PEO status indication LED
 */
#define LED_PIN                         	GPIO_Pin_1
/**
 * @brief Transparent heat element on/off button
 */
#define PEO_ON_BUTTON_PIN                	GPIO_Pin_5
/**
 * @brief 1S1P accumulator battery DCDC converter button
 */
#define S1_DCDC_ON_BUTTON_PIN				GPIO_Pin_2
/**
 * @brief Power on button
 */
#define PWR_ON_BUTTON_PIN                	GPIO_Pin_0
#define PWR_ON_BUTTON_GPIO_PORT          	GPIOA
#define PWR_ON_BUTTON_GPIO_CLK           	RCC_AHBPeriph_GPIOA
#define PWR_ON_BUTTON_EXTI_LINE          	EXTI_Line0
#define PWR_ON_BUTTON_EXTI_PORT_SOURCE   	EXTI_PortSourceGPIOA
#define PWR_ON_BUTTON_EXTI_PIN_SOURCE    	EXTI_PinSource0
#define PWR_ON_BUTTON_EXTI_IRQn          	EXTI0_1_IRQn 

#define BUTTONn	3
