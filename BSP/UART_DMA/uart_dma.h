#ifndef UART_DMA_H
#define UART_DMA_H

#include <stdint.h>
#include <stdbool.h>

/* UART0 (UART_0) DMA TX */
void uart0_dma_init(void);
bool uart0_dma_send(const uint8_t *data, uint16_t len);
bool uart0_dma_is_busy(void);

/* UART1 (UART_1) DMA TX */
void uart1_dma_init(void);
bool uart1_dma_send(const uint8_t *data, uint16_t len);
bool uart1_dma_is_busy(void);

/* UART4 (UART_4) DMA TX */
void uart4_dma_init(void);
bool uart4_dma_send(const uint8_t *data, uint16_t len);
bool uart4_dma_is_busy(void);

/* UART3 DMA TX */
void uart3_dma_init(void);
bool uart3_dma_send(const uint8_t *data, uint16_t len);
bool uart3_dma_is_busy(void);

#endif
