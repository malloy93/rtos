#pragma once
#include <cstdio>

// Host-side log redirect — definiowane PRZED Logger.hpp z RTOS/
// Dzięki #ifndef guard w Logger.hpp, te definicje mają pierwszeństwo.
// Logi z MemoryPool.cpp trafiają na stdout zamiast na UART/DMA.

#define LOG_KERNEL(fmt, ...)            \
    do                                  \
    {                                   \
        printf("[KERNEL] ");            \
        printf(fmt, ##__VA_ARGS__);     \
        printf("\n");                   \
    } while (0)
#define LOG_DEBUG(fmt, ...)             \
    do                                  \
    {                                   \
        printf("[DEBUG]  ");            \
        printf(fmt, ##__VA_ARGS__);     \
        printf("\n");                   \
    } while (0)
#define LOG_INFO(fmt, ...)              \
    do                                  \
    {                                   \
        printf("[INFO]   ");            \
        printf(fmt, ##__VA_ARGS__);     \
        printf("\n");                   \
    } while (0)
#define LOG_ERROR(fmt, ...)             \
    do                                  \
    {                                   \
        printf("[ERROR]  ");            \
        printf(fmt, ##__VA_ARGS__);     \
        printf("\n");                   \
    } while (0)
