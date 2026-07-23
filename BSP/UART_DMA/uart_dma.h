#ifndef UART_DMA_H
#define UART_DMA_H

#include <stdbool.h>
#include <stdint.h>

void uart4_dma_init(void);
bool uart4_dma_send(const uint8_t *data, uint16_t len);
bool uart4_dma_is_busy(void);

uint16_t uart4_rx_available(void);
bool uart4_rx_line_available(void);
uint16_t uart4_rx_read_line(char *buf, uint16_t maxlen);
void uart4_rx_flush(void);

#endif
