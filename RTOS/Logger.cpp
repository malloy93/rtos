#include "Logger.hpp"

#include <cstdio>
#include <cstring>

namespace kernel
{

// void Logger::sendMessage(const char* msg)
// {
//     char buffer[256];
//     snprintf(buffer, sizeof(buffer), "%s\r\n", msg);
//     auto size = strlen(buffer);
//     HAL_UART_Transmit(&comPort, reinterpret_cast<const uint8_t*>(buffer), size, HAL_MAX_DELAY);
// }

} // namespace kernel