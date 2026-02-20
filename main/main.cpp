
#include "main.hpp"
#include <memory>
#include <vector>
#include "RTOS/Core.hpp"
#include "RTOS/Logger.hpp"
#include "RTOS/SysApi.hpp"
#include "dma.hpp"
#include "gpio.hpp"
#include "usart.h"
volatile uint32_t thread_switch_counter = 0;

int x{0};
volatile uint32_t counters[3];

void SystemClock_Config(void);

bool blocker{false};
bool blockers{false};
bool block{false};

void task0();
void task1();
void task2();
void task4();
void task5();

core::RTCore* rtKernel;

void Test_Send_String_DMA(void)
{
    // KROK 1: Dane muszą być STATIC (lub globalne).
    // Dlaczego? Funkcja HAL_UART_Transmit_DMA wraca natychmiast (nie blokuje).
    // Gdyby to była zwykła tablica 'char buf[]', przestałaby istnieć po wyjściu z tej funkcji,
    // a DMA wysyłałoby śmieci z pamięci RAM.
    static const char message[] = "To jest dlugi tekst wyslany przez DMA!\r\n";

    // KROK 2: Sprawdzamy, czy poprzednia transmisja się zakończyła.
    // Bez tego, jeśli wywołasz funkcję za szybko, HAL zwróci HAL_BUSY i nic nie wyśle.
    if (huart1.gState == HAL_UART_STATE_READY)
    {
        // KROK 3: Uruchomienie DMA
        // Rzutujemy (char*) na (uint8_t*), bo tego wymaga HAL.
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)message, strlen(message));
    }
}

int main(void)

{
    SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;
    SCB->CPACR &= ~(0xF << 20);
    // std::vector<void (*)()> threads;
    // threads.push_back(task0);
    // threads.push_back(task1);

    // // uint32_t pool2[24468];
    // threads.push_back(task2);
    // threads.push_back(task4);
    // threads.push_back(task5);

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    DMA_Init();
    MX_USART1_UART_Init();
    core::Logger::init(&huart1, core::LogLevel::DEBUG);
    LOG_INFO("Logger initialized");
    utils::IdGen idGen;
    core::RTCore::init(idGen);
    rtKernel = core::RTCore::getInstance();
    // Test_Send_String_DMA();
    // rtKernel->addThreads(threads);
    // rtKernel->addSystemThreads();
    rtKernel->add(task0);
    rtKernel->add(task1);
    rtKernel->add(task2);
    rtKernel->add(task4);
    rtKernel->add(task5);
    rtKernel->launch(10u);

    while (1)
    {
    }
}
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Konfiguracja zasilania */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** * KLUCZOWE: Używamy BYPASS, bo zegar przychodzi z ST-LINKa.
     * HSE = 8 MHz (zewnętrzny sygnał)
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON; // BYPASS zamiast ON!
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    // Obliczenia dla 180 MHz:
    // Wejście: 8 MHz
    // PLLM = 4   -> VCO Input = 8 / 4 = 2 MHz (Musi być 1~2 MHz)
    // PLLN = 180 -> VCO Output = 2 * 180 = 360 MHz
    // PLLP = 2   -> SysClk = 360 / 2 = 180 MHz
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 180;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7; // USB wymaga 48MHz (360/7 = 51.4 - trochę krzywo, dla USB lepsze N=336)
                                    // Ale jeśli nie używasz USB w procku, Q=7 lub Q=4 jest OK.

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        // Jeśli tu wpadasz, sprawdź czy zworki SB16/17/19/20 są zwarte
        // i czy płytka to na pewno DISC1 (MB1075)
        Error_Handler();
    }

    /** Aktywacja Over-Drive dla 180 MHz */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        Error_Handler();
    }

    /** Zegary szyn (Bus Clocks) */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; // 45 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; // 90 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}
// void SystemClock_Config(void)
// {
//     RCC_OscInitTypeDef RCC_OscInitStruct{0}; // = {0};
//     RCC_ClkInitTypeDef RCC_ClkInitStruct{0};

//     /** Configure the main internal regulator output voltage
//      */
//     __HAL_RCC_PWR_CLK_ENABLE();
//     __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

//     /** Initializes the RCC Oscillators according to the specified parameters
//      * in the RCC_OscInitTypeDef structure.
//      */
//     RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//     RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
//     // RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//     RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//     RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//     RCC_OscInitStruct.PLL.PLLM = 4;
//     RCC_OscInitStruct.PLL.PLLN = 180;
//     RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//     RCC_OscInitStruct.PLL.PLLQ = 4;
//     if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//     {
//         Error_Handler();
//     }

//     /** Activate the Over-Drive mode
//      */
//     if (HAL_PWREx_EnableOverDrive() != HAL_OK)
//     {
//         Error_Handler();
//     }

//     /** Initializes the CPU, AHB and APB buses clocks
//      */
//     RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
//     RCC_CLOCKTYPE_PCLK2; RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; RCC_ClkInitStruct.AHBCLKDivider =
//     RCC_SYSCLK_DIV1; RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; RCC_ClkInitStruct.APB2CLKDivider =
//     RCC_HCLK_DIV2;

//     if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
//     {
//         Error_Handler();
//     }
// }

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
            // core::removeTask(id);
            // rtKernel->remove(id);
        }
        if (thread_switch_counter > 200 and not isRemoved)
        {
            triggeredCounter = thread_switch_counter;
            isRemoved = true;
            // core::addTask(task0);
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
        LOG_DEBUG("Thread 1");
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
            LOG_DEBUG("Thread 5 is running");
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

extern DMA_HandleTypeDef hdma_usart1_tx;

extern "C"
{
    // Ta nazwa MUSI być identyczna jak w pliku startup.s
    void DMA2_Stream7_IRQHandler(void)
    {
        // Przekazujemy obsługę do HAL (czyszczenie flag, wywołanie callbacków)
        HAL_DMA_IRQHandler(&hdma_usart1_tx);
    }
}
#endif /* USE_FULL_ASSERT */
