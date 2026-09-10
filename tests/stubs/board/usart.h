#pragma once
#include <stdint.h>

/* Minimal HAL UART stubs for host-side unit tests */

typedef enum { HAL_OK = 0 } HAL_StatusTypeDef;
typedef enum { HAL_UART_STATE_READY = 0x20U } HAL_UART_StateTypeDef;

typedef struct
{
    HAL_UART_StateTypeDef gState;
} UART_HandleTypeDef;

static inline HAL_StatusTypeDef HAL_UART_Transmit(
    UART_HandleTypeDef* h, const uint8_t* p, uint16_t s, uint32_t t)
{ (void)h; (void)p; (void)s; (void)t; return HAL_OK; }

static inline HAL_StatusTypeDef HAL_UART_Transmit_DMA(
    UART_HandleTypeDef* h, const uint8_t* p, uint16_t s)
{ (void)h; (void)p; (void)s; return HAL_OK; }
