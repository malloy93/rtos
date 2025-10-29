
#include "main.hpp"
#include <memory>
#include <vector>
#include "RTOS/Kernel.hpp"
#include "RTOS/Logger.hpp"
#include "gpio.hpp"
#include "usart.h"

volatile uint32_t thread_switch_counter = 0;

int x{0};
volatile uint32_t counters[3];

// core::Logger* logger;

// core::RTCore* rtKernel;

void SystemClock_Config(void);

bool blocker{false};
bool blockers{false};
bool block{false};

void task0();
void task1();
void task2();
void task4();
void task5();

int main(void)
{
    std::vector<void (*)()> threads;
    threads.push_back(task0);
    threads.push_back(task1);
    // threads.push_back(task2);
    // threads.push_back(task4);
    // threads.push_back(task5);

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    core::Logger::init(&huart1, core::LogLevel::DEBUG);
    LOG_INFO("Logger initialized");
    utils::IdGen idGen;
    // rtKernel = new kernel::RTCore(*logger, idGen);
    core::RTCore::init(idGen);
    auto rtKernel = core::RTCore::getInstance();
    rtKernel->addThreads(threads);
    rtKernel->launch(10u);

    while (1)
    {
    }

    // delete rtKernel;
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct{0}; // = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct{0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 180;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Activate the Over-Drive mode
     */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

void task0()
{
    while (true)
    {
        HAL_GPIO_TogglePin(redLed_GPIO_Port, redLed_Pin);
        counters[0]++;
    }
}
void task1()
{
    bool isTriggered{false};
    bool isRemoved{false};
    uint32_t removedCounter{0};
    uint32_t triggeredCounter{0};
    uint16_t id{0};
    while (true)
    {
        if (thread_switch_counter > 100 and not isTriggered)
        {
            removedCounter = thread_switch_counter;
            isTriggered = true;
            core::removeTask(id);
            // rtKernel->remove(id);
        }
        if (thread_switch_counter > 200 and not isRemoved)
        {
            triggeredCounter = thread_switch_counter;
            isRemoved = true;
            core::addTask(task0);
            //
            // id = rtKernel->add(task0);
        }
        if (thread_switch_counter - removedCounter > 300 and isTriggered)
        {
            // core::addTask(task2);
            isTriggered = false;
        }
        if (thread_switch_counter - triggeredCounter > 500 and isRemoved)
        {
            isRemoved = false;
        }
        // LOG_INFO("Thread %d", 1);
        HAL_GPIO_TogglePin(greenLed_GPIO_Port, greenLed_Pin);
        HAL_Delay(500);
        counters[1]++;
    }
}
void task2()
{
    while (true)
    {
        // LOG_INFO("Thread 2");
        HAL_GPIO_TogglePin(redLed_GPIO_Port, redLed_Pin);
        HAL_Delay(500);
        counters[2]++;
    }
}
void task4()
{
    while (true)
    {
        // LOG_INFO("Thread 3");
        HAL_GPIO_TogglePin(redLed_GPIO_Port, redLed_Pin);
        HAL_Delay(500);
        counters[2]++;
        if (not block)
        {
            // rtKernel->addThread(task5);
            block = true;
        }
    }
}

void task5()
{
    while (true)
    {
        // LOG_INFO("Thread 4");
        if (not blockers)
        {
            // LOG_DEBUG("Thread 5 is running");
            blockers = true;
        }
    }
}

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM7 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM7)
    {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    NVIC_SystemReset();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
