#pragma once

#include <rtos/LogLevel.hpp>
#include <rtos/Telemetry/UartProtocol.hpp>

namespace core
{

class Logger
{
public:
    static void init(
        UART_HandleTypeDef* uart,
        LogLevel logLevel,
        telemetry::OutputMode mode = telemetry::OutputMode::DATA_AND_LOGS);

    static Logger* getInstance() { return loggerInstance; }
    void log(LogLevel msgLevel, const char* fmt, ...);
    void send() { protocol.processTx(); }

    telemetry::UartProtocol& uartProtocol() { return protocol; }

private:
    LogLevel logLevel;
    inline static Logger* loggerInstance = nullptr;
    telemetry::UartProtocol protocol;

    Logger(
        UART_HandleTypeDef* uart,
        LogLevel minimumLevel,
        telemetry::OutputMode mode);
};
} // namespace core

#ifndef LOG_KERNEL
#define LOG_KERNEL(fmt, ...)                                        \
    do                                                              \
    {                                                               \
        auto* L = core::Logger::getInstance();                      \
        if (L) L->log(core::LogLevel::KERNEL, fmt, ##__VA_ARGS__);  \
    } while (0)
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt, ...)                                        \
    do                                                             \
    {                                                              \
        auto* L = core::Logger::getInstance();                     \
        if (L) L->log(core::LogLevel::DEBUG, fmt, ##__VA_ARGS__);  \
    } while (0)
#endif

#ifndef LOG_INFO
#define LOG_INFO(fmt, ...)                                        \
    do                                                            \
    {                                                             \
        auto* L = core::Logger::getInstance();                    \
        if (L) L->log(core::LogLevel::INFO, fmt, ##__VA_ARGS__);  \
    } while (0)
#endif

#ifndef LOG_ERROR
#define LOG_ERROR(fmt, ...)                                        \
    do                                                             \
    {                                                              \
        auto* L = core::Logger::getInstance();                     \
        if (L) L->log(core::LogLevel::ERROR, fmt, ##__VA_ARGS__);  \
    } while (0)
#endif
