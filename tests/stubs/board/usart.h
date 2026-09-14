#pragma once
#include <stdint.h>

/* Minimal HAL UART stubs for host-side unit tests */

typedef enum { HAL_OK = 0, HAL_BUSY = 2 } HAL_StatusTypeDef;
typedef enum { HAL_UART_STATE_READY = 0x20U } HAL_UART_StateTypeDef;

typedef struct
{
    HAL_UART_StateTypeDef gState;
} UART_HandleTypeDef;

#ifdef __cplusplus
extern "C" {
#endif

static inline HAL_StatusTypeDef HAL_UART_Transmit(
    UART_HandleTypeDef* h, const uint8_t* p, uint16_t s, uint32_t t)
{ (void)h; (void)p; (void)s; (void)t; return HAL_OK; }

HAL_StatusTypeDef HAL_UART_Transmit_DMA(
    UART_HandleTypeDef* h, const uint8_t* p, uint16_t s);

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* h);

#ifdef __cplusplus
}
#endif
