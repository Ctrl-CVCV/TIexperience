/* UART4 (PB10/PB11): DMA_CH2 TX and DMA_CH5 RX. */

#include "uart_dma.h"
#include "../../ti_msp_dl_config.h"

#define RX_BUF_SIZE 128U
#define RX_BUF_MASK (RX_BUF_SIZE - 1U)

static volatile uint8_t g_rx_dma_byte;
static volatile uint8_t g_rx_buf[RX_BUF_SIZE];
static volatile uint16_t g_rx_head;
static uint16_t g_rx_tail;
static volatile uint16_t g_rx_count;

static void rx_push(uint8_t data)
{
    g_rx_buf[g_rx_head] = data;
    g_rx_head = (g_rx_head + 1U) & RX_BUF_MASK;

    if (g_rx_count < RX_BUF_SIZE) {
        g_rx_count++;
    } else {
        g_rx_tail = (g_rx_tail + 1U) & RX_BUF_MASK;
    }
}

static void uart4_rx_dma_arm(void)
{
    DL_DMA_disableChannel(DMA, DMA_CH5_CHAN_ID);
    DL_DMA_setSrcAddr(
        DMA, DMA_CH5_CHAN_ID, (uint32_t)&UART_4_INST->RXDATA);
    DL_DMA_setDestAddr(
        DMA, DMA_CH5_CHAN_ID, (uint32_t)&g_rx_dma_byte);
    DL_DMA_setTransferSize(DMA, DMA_CH5_CHAN_ID, 1U);
    DL_DMA_enableChannel(DMA, DMA_CH5_CHAN_ID);
}

void uart4_dma_init(void)
{
    DL_DMA_disableChannel(DMA, DMA_CH2_CHAN_ID);
    DL_DMA_disableChannel(DMA, DMA_CH5_CHAN_ID);

    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_count = 0U;

    NVIC_ClearPendingIRQ(UART_4_INST_INT_IRQN);
    NVIC_SetPriority(UART_4_INST_INT_IRQN, 1U);
    NVIC_EnableIRQ(UART_4_INST_INT_IRQN);

    uart4_rx_dma_arm();
}

bool uart4_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, DMA_CH2_CHAN_ID);
}

bool uart4_dma_send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U || uart4_dma_is_busy()) {
        return false;
    }

    DL_DMA_setSrcAddr(DMA, DMA_CH2_CHAN_ID, (uint32_t)data);
    DL_DMA_setDestAddr(
        DMA, DMA_CH2_CHAN_ID, (uint32_t)&UART_4_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_CH2_CHAN_ID, (uint32_t)len);
    DL_DMA_enableChannel(DMA, DMA_CH2_CHAN_ID);
    return true;
}

void UART_4_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_4_INST)) {
    case DL_UART_MAIN_IIDX_DMA_DONE_RX:
        rx_push(g_rx_dma_byte);
        uart4_rx_dma_arm();
        break;
    default:
        break;
    }
}

uint16_t uart4_rx_available(void)
{
    return g_rx_count;
}

static uint8_t uart4_rx_read_byte(void)
{
    uint8_t data = 0U;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    if (g_rx_count > 0U) {
        data = g_rx_buf[g_rx_tail];
        g_rx_tail = (g_rx_tail + 1U) & RX_BUF_MASK;
        g_rx_count--;
    }
    if ((primask & 1U) == 0U) {
        __enable_irq();
    }
    return data;
}

bool uart4_rx_line_available(void)
{
    uint16_t i;

    for (i = 0U; i < g_rx_count; i++) {
        if (g_rx_buf[(g_rx_tail + i) & RX_BUF_MASK] == '\n') {
            return true;
        }
    }
    return false;
}

uint16_t uart4_rx_read_line(char *buf, uint16_t maxlen)
{
    uint16_t length = 0U;
    uint8_t data;

    if (buf == NULL || maxlen == 0U) {
        return 0U;
    }

    while (length + 1U < maxlen && g_rx_count > 0U) {
        data = uart4_rx_read_byte();
        buf[length++] = (char)data;
        if (data == '\n') {
            break;
        }
    }
    buf[length] = '\0';
    return length;
}

void uart4_rx_flush(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_count = 0U;
    if ((primask & 1U) == 0U) {
        __enable_irq();
    }
}
