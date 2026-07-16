#ifndef UART_DMA_H
#define UART_DMA_H

#include <stdint.h>
#include <stdbool.h>

void uart_dma_init(void);
bool uart_dma_send(const uint8_t *data, uint16_t len);
bool uart_dma_is_busy(void);

#endif
