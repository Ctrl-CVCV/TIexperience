/*
 * uart_dma.c - UART0 DMA TX (DMA_CH0)
 *
 * SysConfig DMA defaults have wrong srcIncrement/srcWidth/destWidth.
 * We init the channel once with correct settings in uart_dma_init(),
 * then only update addresses & size per send.
 */

#include "uart_dma.h"
#include "../../ti_msp_dl_config.h"

void uart_dma_init(void)
{
    DL_DMA_Config cfg = {
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode  = DL_DMA_NORMAL_MODE,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,   /* UART TXDATA: fixed */
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,   /* memory buffer: inc  */
        .destWidth     = DL_DMA_WIDTH_BYTE,
        .srcWidth      = DL_DMA_WIDTH_BYTE,
        .trigger       = UART_0_INST_DMA_TRIGGER,
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
    };

    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID, &cfg);
}

bool uart_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, DMA_CH0_CHAN_ID);
}

bool uart_dma_send(const uint8_t *data, uint16_t len)
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
