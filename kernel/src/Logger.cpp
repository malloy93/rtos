#include <rtos/Logger.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace core
{

void Logger::init(
    UART_HandleTypeDef* uart,
    LogLevel minimumLevel,
    telemetry::OutputMode mode)
{
    if (loggerInstance == nullptr)
    {
        loggerInstance = new Logger(uart, minimumLevel, mode);
    }
}

Logger::Logger(
    UART_HandleTypeDef* uart,
    LogLevel minimumLevel,
    telemetry::OutputMode mode)
    : logLevel(minimumLevel)
{
    protocol.init(uart, mode);
}

void Logger::log(LogLevel msgLevel, const char* fmt, ...)
{
    if (fmt == nullptr || logLevel == LogLevel::OFF) return;
    if (static_cast<uint8_t>(msgLevel) < static_cast<uint8_t>(logLevel)) return;

    char buffer[telemetry::MAX_LOG_TEXT_BYTES + 1U]{};
    va_list arguments;
    va_start(arguments, fmt);
    const int result = vsnprintf(buffer, sizeof(buffer), fmt, arguments);
    va_end(arguments);

    if (result < 0) return;
    const size_t length = std::min(static_cast<size_t>(result), sizeof(buffer) - 1U);
    (void)protocol.sendLog(msgLevel, std::string_view(buffer, length));
}

} // namespace core
