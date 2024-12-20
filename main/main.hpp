#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h"


void Error_Handler(void);


#define greenLed_Pin GPIO_PIN_13
#define greenLed_GPIO_Port GPIOG
#define redLed_Pin GPIO_PIN_14
#define redLed_GPIO_Port GPIOG


#ifdef __cplusplus
}
#endif


