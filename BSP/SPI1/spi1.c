#include "spi1.h"
#include "../bsp.h"

uint8_t spi1_read_write_byte(uint8_t dat)
{
    uint8_t data = 0;

    DL_SPI_transmitData8(SPI_1_INST, dat);
    while (DL_SPI_isBusy(SPI_1_INST));
    data = DL_SPI_receiveData8(SPI_1_INST);
    while (DL_SPI_isBusy(SPI_1_INST));

    return data;
}
