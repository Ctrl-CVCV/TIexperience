/*
 * uart_dma.c - UART0 DMA TX (DMA_CH0)
 *
 * SysConfig DMA defaults are WRONG for UART TX:
 *   srcIncrement = UNCHANGED  → must be INCREMENT
 *   srcWidth     = WORD       → must be BYTE
 *   destWidth    = WORD       → must be BYTE
 * We override these at send time.
 */

#include "uart_dma.h"
#include "../../ti_msp_dl_config.h"

void uart_dma_init(void)
{
    /* DMA CH0 config is done by SYSCFG_DL_DMA_init().
     * We override src/dest width/increment at send time.
     * No interrupt — poll DL_DMA_isChannelEnabled() for completion. */
}

bool uart_dma_is_busy(void)
{
    return DL_DMA_isChannelEnabled(DMA, DMA_CH0_CHAN_ID);
}

bool uart_dma_send(const uint8_t *data, uint16_t len)
{
    DL_DMA_Config cfg;

    if (DL_DMA_isChannelEnabled(DMA, DMA_CH0_CHAN_ID) || len == 0 || data == NULL) {
        return false;
    }

    /* Build correct DMA config for UART TX */
    cfg.transferMode  = DL_DMA_SINGLE_TRANSFER_MODE;
    cfg.extendedMode  = DL_DMA_NORMAL_MODE;
    cfg.destIncrement = DL_DMA_ADDR_UNCHANGED;   /* UART TXDATA: fixed addr  */
    cfg.srcIncrement  = DL_DMA_ADDR_INCREMENT;   /* memory buffer: increment */
    cfg.destWidth     = DL_DMA_WIDTH_BYTE;       /* UART is 8-bit            */
    cfg.srcWidth      = DL_DMA_WIDTH_BYTE;
    cfg.trigger       = UART_0_INST_DMA_TRIGGER;
    cfg.triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL;

    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID, &cfg);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)data);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID,
                       (uint32_t)&UART_0_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, len);

    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
    return true;
}
