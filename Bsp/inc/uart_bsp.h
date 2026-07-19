/**
  ******************************************************************************
  * @file    uart_bsp.h
  * @brief   UART7 DMA + idle-line reception framework.
  ******************************************************************************
  */
#ifndef __UART_BSP_H__
#define __UART_BSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"

/* Size of the DMA RX buffer: one idle event delivers at most this many bytes.
   If more bytes arrive without an idle gap, the buffer-full (TC) event fires
   first and the data is delivered as two frames. */
#define UART7_RX_BUF_SIZE    256u

/* Max number of frames buffered between the RX ISR and the consumer task. */
#define UART7_RX_QUEUE_LEN   4u

typedef struct
{
    uint16_t len;                       /* number of valid bytes in data      */
    uint8_t  data[UART7_RX_BUF_SIZE];   /* frame payload (copied from DMA)    */
} uart7_rx_frame_t;

/* Item type: uart7_rx_frame_t. Filled by the RX ISR, consume with
   xQueueReceive(uart7_rx_queue, &frame, timeout). */
extern QueueHandle_t uart7_rx_queue;

/* Create the RX queue and arm the first DMA idle-line reception.
   Call once from a task (or after the scheduler is about to start). */
void uart7_rx_init(void);

/* Non-blocking DMA transmit. Returns HAL_BUSY while the previous transfer
   is still running. NOTE: data must stay valid until TX DMA completes. */
HAL_StatusTypeDef uart7_send_dma(const uint8_t *data, uint16_t len);

/* Diagnostics: frames dropped because the queue was full / UART error count. */
uint32_t uart7_rx_get_drop_cnt(void);
uint32_t uart7_rx_get_err_cnt(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_BSP_H__ */
