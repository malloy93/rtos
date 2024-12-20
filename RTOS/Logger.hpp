#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "Core/Inc/usart.h"

namespace kernel
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
    Logger(UART_HandleTypeDef& uart, LogLevel logLevel) : comPort(uart), logLevel(logLevel)
    {
        constexpr const char* startMsg = "\r\n \r\n RRTOS ver. 0.323 \r\n";
        HAL_UART_Transmit(&comPort, reinterpret_cast<const uint8_t*>(startMsg), 24, HAL_MAX_DELAY);
    }

    void logError(const char* msg, ...)
    {
        switch (logLevel)
        {
            case LogLevel::KERNEL:
            case LogLevel::DEBUG:
            case LogLevel::INFO:
            case LogLevel::ERROR:
            {
                char buffer[256];
                int offset = snprintf(buffer, sizeof(buffer), "ERR:  ");
                va_list args;
                va_start(args, msg);
                int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, msg, args);
                va_end(args);
                snprintf(buffer + offset + len, sizeof(buffer) - offset - len, "\r\n");
                sendMessage(buffer);
                break;
            }
            case LogLevel::OFF:
            default:
                break;
        }
    }

    void logInfo(const char* msg, ...)
    {
        switch (logLevel)
        {
            case LogLevel::KERNEL:
            case LogLevel::DEBUG:
            case LogLevel::INFO:
            {
                char buffer[256];
                int offset = snprintf(buffer, sizeof(buffer), "INF:  ");
                va_list args;
                va_start(args, msg);
                int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, msg, args);
                va_end(args);
                snprintf(buffer + offset + len, sizeof(buffer) - offset - len, "\r\n");
                sendMessage(buffer);
                break;
            }
            case LogLevel::ERROR:
            case LogLevel::OFF:
            default:
                break;
        }
    }

    void logDebug(const char* msg, ...)
    {
        switch (logLevel)
        {
            case LogLevel::KERNEL:
            case LogLevel::DEBUG:
            {
                char buffer[256];
                int offset = snprintf(buffer, sizeof(buffer), "DBG:  ");
                va_list args;
                va_start(args, msg);
                int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, msg, args);
                va_end(args);
                snprintf(buffer + offset + len, sizeof(buffer) - offset - len, "\r\n");
                sendMessage(buffer);
                break;
            }
            case LogLevel::INFO:
            case LogLevel::ERROR:
            case LogLevel::OFF:
            default:
                break;
        }
    }

    void logKernel(const char* msg, ...)
    {
        switch (logLevel)
        {
            case LogLevel::KERNEL:
            {
                char buffer[256];
                int offset = snprintf(buffer, sizeof(buffer), "KRL:  ");
                va_list args;
                va_start(args, msg);
                int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, msg, args);
                va_end(args);
                snprintf(buffer + offset + len, sizeof(buffer) - offset - len, "\r\n");
                sendMessage(buffer);
                break;
            }
            case LogLevel::DEBUG:
            case LogLevel::INFO:
            case LogLevel::ERROR:
            case LogLevel::OFF:
            default:
                break;
        }
    }

private:
    UART_HandleTypeDef& comPort;
    LogLevel logLevel;

    void sendMessage(const char* buffer)
    {
        HAL_UART_Transmit(&comPort, reinterpret_cast<const uint8_t*>(buffer), strlen(buffer), HAL_MAX_DELAY);
    }
};
} // namespace kernel
