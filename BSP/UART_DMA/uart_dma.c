/*
 * uart_dma.c - UART0 & UART1 DMA TX
 *
 * SysConfig-generated DMA defaults have wrong srcIncrement/srcWidth/destWidth.
 * We init each channel once with correct settings, then only update addresses
 * and size per send.
 */

#include "uart_dma.h"
#include "../../ti_msp_dl_config.h"

/* ========== UART0 (UART_0, PA10/PA11, DMA_CH0) ========== */

void uart0_dma_init(void)
{
    DL_DMA_Config cfg = {
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode  = DL_DMA_NORMAL_MODE,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,   /* UART TXDATA: fixed */
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,   /* memory buffer: inc  */
        .destWidth     = DL_DMA_WIDTH_BYTE,
        .srcWidth      = DL_DMA_WIDTH_BYTE,
        .trigger       = UART_0_INST_DMA_TRIGGER_0,
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
    };

    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID, &cfg);
}

bool uart0_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, DMA_CH0_CHAN_ID);
}

bool uart0_dma_send(const uint8_t *data, uint16_t len)
{
    if (DL_DMA_isChannelEnabled(DMA, DMA_CH0_CHAN_ID) || len == 0 || data == NULL) {
        return false;
    }

    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)data);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID,
                       (uint32_t)&UART_0_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, len);

    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
    return true;
}

/* ========== UART1 (UART_1, PB6/PB5, DMA_CH4) ========== */

void uart1_dma_init(void)
{
    DL_DMA_Config cfg = {
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode  = DL_DMA_NORMAL_MODE,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,
        .destWidth     = DL_DMA_WIDTH_BYTE,
        .srcWidth      = DL_DMA_WIDTH_BYTE,
        .trigger       = UART_1_INST_DMA_TRIGGER_1,
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
    };

    DL_DMA_disableChannel(DMA, DMA_CH4_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL4);
    DL_DMA_initChannel(DMA, DMA_CH4_CHAN_ID, &cfg);
}

bool uart1_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, DMA_CH4_CHAN_ID);
}

bool uart1_dma_send(const uint8_t *data, uint16_t len)
{
    if (DL_DMA_isChannelEnabled(DMA, DMA_CH4_CHAN_ID) || len == 0 || data == NULL) {
        return false;
    }

    DL_DMA_setSrcAddr(DMA, DMA_CH4_CHAN_ID, (uint32_t)data);
    DL_DMA_setDestAddr(DMA, DMA_CH4_CHAN_ID,
                       (uint32_t)&UART_1_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_CH4_CHAN_ID, len);

    DL_DMA_enableChannel(DMA, DMA_CH4_CHAN_ID);
    return true;
}

/* ========== UART4 (UART_4, PB10/PB11, DMA_CH2) ========== */

void uart4_dma_init(void)
{
    DL_DMA_Config cfg = {
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode  = DL_DMA_NORMAL_MODE,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,   /* UART TXDATA: fixed */
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,   /* memory buffer: inc  */
        .destWidth     = DL_DMA_WIDTH_BYTE,
        .srcWidth      = DL_DMA_WIDTH_BYTE,
        .trigger       = UART_4_INST_DMA_TRIGGER_1,
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
    };

    DL_DMA_disableChannel(DMA, DMA_CH2_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL2);
    DL_DMA_initChannel(DMA, DMA_CH2_CHAN_ID, &cfg);
}

bool uart4_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, DMA_CH2_CHAN_ID);
}

bool uart4_dma_send(const uint8_t *data, uint16_t len)
{
    if (DL_DMA_isChannelEnabled(DMA, DMA_CH2_CHAN_ID) || len == 0 || data == NULL) {
        return false;
    }

    DL_DMA_setSrcAddr(DMA, DMA_CH2_CHAN_ID, (uint32_t)data);
    DL_DMA_setDestAddr(DMA, DMA_CH2_CHAN_ID,
                       (uint32_t)&UART_4_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_CH2_CHAN_ID, len);

    DL_DMA_enableChannel(DMA, DMA_CH2_CHAN_ID);
    return true;
}

/* ========== UART3 (DMA_CH6) ========== */

#define UART_3_DMA_CHAN_ID   (6)

void uart3_dma_init(void)
{
    DL_DMA_Config cfg = {
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode  = DL_DMA_NORMAL_MODE,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,
        .destWidth     = DL_DMA_WIDTH_BYTE,
        .srcWidth      = DL_DMA_WIDTH_BYTE,
        .trigger       = DMA_UART3_TX_TRIG,
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
    };

    DL_DMA_disableChannel(DMA, UART_3_DMA_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL6);
    DL_DMA_initChannel(DMA, UART_3_DMA_CHAN_ID, &cfg);
}

bool uart3_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, UART_3_DMA_CHAN_ID);
}

bool uart3_dma_send(const uint8_t *data, uint16_t len)
{
    if (DL_DMA_isChannelEnabled(DMA, UART_3_DMA_CHAN_ID) || len == 0 || data == NULL) {
        return false;
    }

    DL_DMA_setSrcAddr(DMA, UART_3_DMA_CHAN_ID, (uint32_t)data);
    DL_DMA_setDestAddr(DMA, UART_3_DMA_CHAN_ID,
                       (uint32_t)&UART3->TXDATA);
    DL_DMA_setTransferSize(DMA, UART_3_DMA_CHAN_ID, len);

    DL_DMA_enableChannel(DMA, UART_3_DMA_CHAN_ID);
    return true;
}
