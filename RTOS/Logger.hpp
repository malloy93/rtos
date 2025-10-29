#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "Core/Inc/usart.h"

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
        if (msgLevel == LogLevel::OFF || logLevel == LogLevel::OFF) return;
        // filtr jak w oryginale: przepuszczamy gdy msgLevel >= level_
        if ((uint8_t)msgLevel < (uint8_t)logLevel) return;

        const char* tag = "";
        switch (msgLevel)
        {
            case LogLevel::KERNEL:
                tag = "KRL: ";
                break;
            case LogLevel::DEBUG:
                tag = "DBG: ";
                break;
            case LogLevel::INFO:
                tag = "INF: ";
                break;
            case LogLevel::ERROR:
                tag = "ERR: ";
                break;
            default:
                break;
        }

        char buf[256];
        int off = snprintf(buf, sizeof(buf), "%s", tag);

        va_list ap;
        va_start(ap, fmt);
        int n = vsnprintf(buf + off, sizeof(buf) - off, fmt, ap);
        va_end(ap);
        if (n < 0) n = 0;
        size_t used = static_cast<size_t>(off + (n >= 0 ? n : 0));
        if (used + 2 <= sizeof(buf))
        {
            buf[used++] = '\r';
            buf[used++] = '\n';
        }
        else if (used < sizeof(buf))
        {
            buf[used++] = '\n';
        }

        HAL_UART_Transmit(comPort, reinterpret_cast<const uint8_t*>(buf), static_cast<uint16_t>(used), HAL_MAX_DELAY);
    }

private:
    UART_HandleTypeDef* comPort{nullptr};
    LogLevel logLevel;
    inline static Logger* loggerInstance = nullptr;

    Logger(UART_HandleTypeDef* uart, LogLevel logLevel) : comPort(uart), logLevel(logLevel)
    {
        constexpr const char* startMsg = "\r\n \r\n RRTOS ver. 0.4 \r\n";
        HAL_UART_Transmit(comPort, reinterpret_cast<const uint8_t*>(startMsg), 24, HAL_MAX_DELAY);
    }

    void sendMessage(const char* buffer)
    {
        HAL_UART_Transmit(comPort, reinterpret_cast<const uint8_t*>(buffer), strlen(buffer), HAL_MAX_DELAY);
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