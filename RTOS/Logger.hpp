#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "CircularBuffer.hpp"
#include "Core/Inc/usart.h"
#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"
namespace core
{

enum class LogLevel
{
    KERNEL = 0,
    DEBUG = 1,
    INFO = 2,
    ERROR = 3,
    OFF = 4
};

// class RTCore;

class Logger
{
public:
    static void init(UART_HandleTypeDef* uart, LogLevel logLevel)
    {
        if (loggerInstance == nullptr)
        {
            loggerInstance = new Logger(uart, logLevel);
        }
    }
    static Logger* getInstance() { return loggerInstance; }
    void log(LogLevel msgLevel, const char* fmt, ...)
    {
        if (!comPort) return;
        if (logLevel == LogLevel::OFF) return;

        // Filtr poziomu logowania
        if ((uint8_t)msgLevel < (uint8_t)logLevel) return;

        // --- USUNIĘTA SEKCJA SWITCH I TAGI ---

        // 2. Bufor na stosie
        char buf[128];

        // 3. Formatowanie samej wiadomości (bez offsetu)
        va_list ap;
        va_start(ap, fmt);
        // Piszemy od początku bufora (buf), używając całej jego wielkości
        int n = vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

        // 4. Obsługa długości i dodawanie nowej linii
        if (n < 0) n = 0;
        size_t used = static_cast<size_t>(n);

        // Zabezpieczenie przed przepełnieniem przy dodawaniu \r\n
        // Musimy mieć miejsce na co najmniej 2 znaki (\r\n) + ewentualne 0
        if (used >= sizeof(buf) - 2)
        {
            used = sizeof(buf) - 3; // Przycinamy wiadomość, żeby zmieścić \r\n
        }

        buf[used++] = '\r';
        buf[used++] = '\n';

        logBuffer.push(std::span<char>(buf, used));
        // HAL_UART_Transmit(comPort, reinterpret_cast<const uint8_t*>(buf), static_cast<uint16_t>(used), 5);
    }

    void send()
    {
        // 1. Sprawdź, czy UART jest wolny (czy poprzednie DMA skończyło)
        // gState to flaga HAL-a zarządzająca nadawaniem
        if (comPort->gState != HAL_UART_STATE_READY)
        {
            return; // Poprzednia paczka jeszcze leci, wrócimy tu za chwilę
        }

        // 2. Sprawdź, czy mamy nowe dane w buforze
        if (logBuffer.isEmpty())
        {
            return;
        }

        // 3. Pobierz parametry dla DMA
        // To "magia", która obsługuje bufor kołowy w dwóch rzutach
        size_t len = logBuffer.getLinearBlockSize();
        uint8_t* ptr = logBuffer.getReadPtr();

        // 4. Odpal DMA
        if (len > 0)
        {
            // To wywołanie nie blokuje CPU!
            // DMA zaczyna słać w tle.
            if (HAL_UART_Transmit_DMA(comPort, ptr, (uint16_t)len) == HAL_OK)
            {
                // 5. Zakładamy sukces i przesuwamy ogon
                // (W idealnym świecie robimy to w callbacku, ale w prostym loggerze
                //  można "zaliczyć" dane jako wysłane w momencie zlecenia).
                logBuffer.advanceTail(len);
            }
        }
    }

private:
    UART_HandleTypeDef* comPort{nullptr};
    LogLevel logLevel;
    inline static Logger* loggerInstance = nullptr;
    CircularBuffer logBuffer;

    Logger(UART_HandleTypeDef* uart, LogLevel logLevel) : comPort(uart), logLevel(logLevel)
    {
        constexpr const char* startMsg = "\r\n \r\n RRTOS ver. 0.4 \r\n";
        HAL_UART_Transmit(comPort, reinterpret_cast<const uint8_t*>(startMsg), 24, 50);
    }

    void sendMessage(const char* buffer)
    {
        HAL_UART_Transmit(comPort, reinterpret_cast<const uint8_t*>(buffer), strlen(buffer), 50);
    }
};
} // namespace core

#define LOG_KERNEL(fmt, ...)                                       \
    do                                                             \
    {                                                              \
        auto* L = core::Logger::getInstance();                     \
        if (L) L->log(core::LogLevel::KERNEL, fmt, ##__VA_ARGS__); \
    } while (0)
#define LOG_DEBUG(fmt, ...)                                       \
    do                                                            \
    {                                                             \
        auto* L = core::Logger::getInstance();                    \
        if (L) L->log(core::LogLevel::DEBUG, fmt, ##__VA_ARGS__); \
    } while (0)
#define LOG_INFO(fmt, ...)                                       \
    do                                                           \
    {                                                            \
        auto* L = core::Logger::getInstance();                   \
        if (L) L->log(core::LogLevel::INFO, fmt, ##__VA_ARGS__); \
    } while (0)
#define LOG_ERROR(fmt, ...)                                       \
    do                                                            \
    {                                                             \
        auto* L = core::Logger::getInstance();                    \
        if (L) L->log(core::LogLevel::ERROR, fmt, ##__VA_ARGS__); \
    } while (0)