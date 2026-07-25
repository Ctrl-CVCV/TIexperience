#ifndef BSP_H
#define BSP_H

/******************绯荤粺澶存枃浠?**************/
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../ti_msp_dl_config.h"
/*******************************************/

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int

#define CPUCLK_FREQ_MHZ 80

#define BSP_DELAY_MS_DEFINED
static inline void delay_ms(uint32_t ms)
{
    while (ms--) {
        delay_cycles(CPUCLK_FREQ_MHZ * 1000UL);
    }
}

static inline void delay_us(uint32_t us)
{
    while (us--) {
        delay_cycles(CPUCLK_FREQ_MHZ);
    }
}

/******************鐢ㄦ埛鑷畾涔夊ご鏂囦欢*********/
#include "SPI0_LCD/lcd.h"
#include "SPI1/spi1.h"
/*******************************************/

#endif
