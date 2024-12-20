#include "Utils.hpp"

#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"

namespace utils
{

uint32_t getClockFreq()
{
    RCC_ClkInitTypeDef clk_config;
    uint32_t systemClockFreq;

    // Get the current clock configuration
    HAL_RCC_GetClockConfig(&clk_config, (uint32_t*)(0x00000005U));

    // Determine the system clock frequency based on the clock configuration
    switch (clk_config.SYSCLKSource)
    {
        case RCC_SYSCLKSOURCE_PLLCLK:
            systemClockFreq = HAL_RCC_GetSysClockFreq();
            break;
        case RCC_SYSCLKSOURCE_HSI:
            systemClockFreq = HSI_VALUE;
            break;
        case RCC_SYSCLKSOURCE_HSE:
            systemClockFreq = HSE_VALUE;
            break;
        default:
            systemClockFreq = 0;
            break;
    }

    return systemClockFreq;
}


} // namespace utils