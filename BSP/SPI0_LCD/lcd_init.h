#ifndef __LCD_INIT_H
#define __LCD_INIT_H

/*
 * ST7789 LCD 底层驱动头文件。
 * 引脚定义由 SysConfig (project.syscfg) 的 GPIO "LCD" 和 SPI "SPI_0" 生成，
 * 若无则回退到硬编码值（匹配 Car2 硬件接线）。
 */

#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>
#include "ti_msp_dl_config.h"

#ifndef CPUCLK_FREQ_MHZ
#define CPUCLK_FREQ_MHZ 80
#endif

#ifndef BSP_DELAY_MS_DEFINED
static inline void delay_ms(uint32_t ms)
{
    while (ms--) {
        delay_cycles(CPUCLK_FREQ_MHZ * 1000UL);
    }
}
#endif

/* ---- Fallback: if SysConfig didn't generate LCD_* macros ---- */
#ifndef LCD_BLK_PORT
#define LCD_BLK_PORT  GPIOA
#define LCD_BLK_PIN   DL_GPIO_PIN_7
#define LCD_BLK_IOMUX (IOMUX_PINCM14)
#endif
#ifndef LCD_DC_PORT
#define LCD_DC_PORT   GPIOA
#define LCD_DC_PIN    DL_GPIO_PIN_14
#define LCD_DC_IOMUX  (IOMUX_PINCM36)
#endif
#ifndef LCD_RES_PORT
#define LCD_RES_PORT  GPIOC
#define LCD_RES_PIN   DL_GPIO_PIN_0
#define LCD_RES_IOMUX (IOMUX_PINCM74)
#endif

#ifndef u8
typedef uint8_t  u8;
#endif
#ifndef u16
typedef uint16_t u16;
#endif
#ifndef u32
typedef uint32_t u32;
#endif

#define USE_HORIZONTAL 2

#if (USE_HORIZONTAL == 0) || (USE_HORIZONTAL == 1)
#define LCD_W 240
#define LCD_H 280
#else
#define LCD_W 280
#define LCD_H 240
#endif

#define LCD_RES_Clr() DL_GPIO_clearPins(LCD_RES_PORT, LCD_RES_PIN)
#define LCD_RES_Set() DL_GPIO_setPins(LCD_RES_PORT, LCD_RES_PIN)

#define LCD_DC_Clr()  DL_GPIO_clearPins(LCD_DC_PORT, LCD_DC_PIN)
#define LCD_DC_Set()  DL_GPIO_setPins(LCD_DC_PORT, LCD_DC_PIN)

/* CS = hardware tied LOW */
#define LCD_CS_Clr() ((void)0)
#define LCD_CS_Set() ((void)0)

#define LCD_BLK_Clr() DL_GPIO_clearPins(LCD_BLK_PORT, LCD_BLK_PIN)
#define LCD_BLK_Set() DL_GPIO_setPins(LCD_BLK_PORT, LCD_BLK_PIN)

void LCD_GPIO_Init(void);
void LCD_Writ_Bus(u8 dat);
void LCD_WR_DATA8(u8 dat);
void LCD_WR_DATA(u16 dat);
void LCD_WR_REG(u8 dat);
void LCD_Address_Set(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_Init(void);

#endif
