/**
  ******************************************************************************
  * @file    uart_bsp.c
  * @brief   UART7 DMA + idle-line reception framework.
  *
  *          RX data path:
  *            idle line / DMA buffer full
  *              -> HAL_UARTEx_RxEventCallback (ISR context)
  *              -> copy received bytes into uart7_rx_queue, re-arm DMA
  *              -> consumer task blocks on uart7_rx_queue
  *
  *          Implementation notes:
  *          - hdma_uart7_rx is configured in DMA_NORMAL mode, so every IDLE
  *            or buffer-full event ends the HAL Rx process (RxState back to
  *            READY); reception is re-armed inside the callback.
  *          - The DMA half-transfer interrupt is disabled after arming, so
  *            the event callback only fires on IDLE or buffer-full.
  *          - UART7 and DMA1_Stream1 IRQs share preempt priority 5
  *            (= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY), which makes
  *            the FromISR calls legal and means the two ISRs never nest,
  *            so the shared staging buffer needs no extra locking.
  *          - All RW/ZI data of this project currently links to AXI SRAM
  *            (0x24000000) which DMA1 can access, and D-Cache is disabled.
  *            If you enable D-Cache later, invalidate uart7_rx_dma_buf
  *            before reading it (or place it in a non-cacheable MPU region).
  ******************************************************************************
  */
#include "uart_bsp.h"
#include "usart.h"
#include <string.h>

#include "task.h"

QueueHandle_t uart7_rx_queue = NULL;

static uint8_t uart7_rx_dma_buf[UART7_RX_BUF_SIZE];     /* DMA target buffer  */
static uart7_rx_frame_t uart7_rx_staging;               /* ISR staging area   */

static StaticQueue_t uart7_rx_queue_cb;
static uint8_t uart7_rx_queue_storage[UART7_RX_QUEUE_LEN * sizeof(uart7_rx_frame_t)];

static volatile uint32_t uart7_rx_drop_cnt = 0;
static volatile uint32_t uart7_rx_err_cnt  = 0;

/**
 * @brief Arm (or re-arm) DMA idle-line reception on UART7.
 */
static void uart7_rx_arm(void)
{
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart7, uart7_rx_dma_buf, UART7_RX_BUF_SIZE) == HAL_OK)
    {
        /* Only IDLE / buffer-full events are wanted, drop half-transfer */
        __HAL_DMA_DISABLE_IT(huart7.hdmarx, DMA_IT_HT);
    }
}

/**
 * @brief Create the RX queue and start reception. Call once from a task.
 */
void uart7_rx_init(void)
{
    if (uart7_rx_queue == NULL)
    {
        uart7_rx_queue = xQueueCreateStatic(UART7_RX_QUEUE_LEN,
                                            sizeof(uart7_rx_frame_t),
                                            uart7_rx_queue_storage,
                                            &uart7_rx_queue_cb);
    }
    uart7_rx_arm();
}

/**
 * @brief Non-blocking DMA transmit on UART7.
 * @note  data must stay valid until the TX DMA transfer completes.
 */
HAL_StatusTypeDef uart7_send_dma(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0))
    {
        return HAL_ERROR;
    }
    return HAL_UART_Transmit_DMA(&huart7, (uint8_t *)data, len);
}

uint32_t uart7_rx_get_drop_cnt(void)
{
    return uart7_rx_drop_cnt;
}

uint32_t uart7_rx_get_err_cnt(void)
{
    return uart7_rx_err_cnt;
}

/**
 * @brief Reception event callback (IDLE or buffer full), ISR context.
 * @param Size number of bytes received in uart7_rx_dma_buf
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == UART7)
    {
        BaseType_t woken = pdFALSE;

        /* Secure the data first, then re-arm reception as soon as possible */
        uart7_rx_staging.len = Size;
        memcpy(uart7_rx_staging.data, uart7_rx_dma_buf, Size);

        uart7_rx_arm();

        if (uart7_rx_queue != NULL)
        {
            if (xQueueSendFromISR(uart7_rx_queue, &uart7_rx_staging, &woken) != pdTRUE)
            {
                uart7_rx_drop_cnt++;    /* consumer too slow, frame dropped */
            }
        }
        portYIELD_FROM_ISR(woken);
    }
}

/**
 * @brief UART error callback (overrun / frame / noise error), ISR context.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART7)
    {
        uart7_rx_err_cnt++;

        /* Blocking errors (e.g. overrun with DMA) abort the Rx process and
           RxState returns to READY: reception is dead, re-arm it.
           Non-blocking errors (frame/noise) keep DMA running: do nothing. */
        if (huart->RxState == HAL_UART_STATE_READY)
        {
            uart7_rx_arm();
        }
    }
}
