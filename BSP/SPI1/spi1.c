#include "SPI1/spi1.h"

#define SPI1_TRANSFER_TIMEOUT_LOOP (100000U)

static bool spi1_wait_tx_ready(void)
{
    uint32_t timeout = SPI1_TRANSFER_TIMEOUT_LOOP;

    while (DL_SPI_isTXFIFOFull(SPI_1_INST)) {
        if (timeout == 0U) {
            return false;
        }
        timeout--;
    }

    return true;
}

static bool spi1_wait_rx_ready(void)
{
    uint32_t timeout = SPI1_TRANSFER_TIMEOUT_LOOP;

    while (DL_SPI_isRXFIFOEmpty(SPI_1_INST)) {
        if (timeout == 0U) {
            return false;
        }
        timeout--;
    }

    return true;
}

static bool spi1_wait_idle(void)
{
    uint32_t timeout = SPI1_TRANSFER_TIMEOUT_LOOP;

    while (DL_SPI_isBusy(SPI_1_INST)) {
        if (timeout == 0U) {
            return false;
        }
        timeout--;
    }

    return true;
}

static void spi1_flush_rx_fifo(void)
{
    uint32_t guard = 16U;

    while (!DL_SPI_isRXFIFOEmpty(SPI_1_INST) && (guard > 0U)) {
        (void)DL_SPI_receiveData8(SPI_1_INST);
        guard--;
    }
}

bool spi1_transfer_byte(uint8_t tx_data, uint8_t *rx_data)
{
    uint8_t dummy_rx;

    if (rx_data == NULL) {
        rx_data = &dummy_rx;
    }

    if (!spi1_wait_idle()) {
        return false;
    }

    spi1_flush_rx_fifo();

    if (!spi1_wait_tx_ready()) {
        return false;
    }

    DL_SPI_transmitData8(SPI_1_INST, tx_data);

    if (!spi1_wait_rx_ready()) {
        return false;
    }

    *rx_data = DL_SPI_receiveData8(SPI_1_INST);

    return spi1_wait_idle();
}

uint8_t spi1_read_write_byte(uint8_t dat)
{
    uint8_t data = 0xFFU;

    (void)spi1_transfer_byte(dat, &data);

    return data;
}
