/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G351X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G351X
#define CONFIG_MSPM0G3519

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2500
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for PWMA */
#define PWMA_INST                                                          TIMG6
#define PWMA_INST_IRQHandler                                    TIMG6_IRQHandler
#define PWMA_INST_INT_IRQN                                      (TIMG6_INT_IRQn)
#define PWMA_INST_CLK_FREQ                                              80000000
/* GPIO defines for channel 0 */
#define GPIO_PWMA_C0_PORT                                                  GPIOA
#define GPIO_PWMA_C0_PIN                                          DL_GPIO_PIN_29
#define GPIO_PWMA_C0_IOMUX                                        (IOMUX_PINCM4)
#define GPIO_PWMA_C0_IOMUX_FUNC                       IOMUX_PINCM4_PF_TIMG6_CCP0
#define GPIO_PWMA_C0_IDX                                     DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWMA_C1_PORT                                                  GPIOA
#define GPIO_PWMA_C1_PIN                                          DL_GPIO_PIN_30
#define GPIO_PWMA_C1_IOMUX                                        (IOMUX_PINCM5)
#define GPIO_PWMA_C1_IOMUX_FUNC                       IOMUX_PINCM5_PF_TIMG6_CCP1
#define GPIO_PWMA_C1_IDX                                     DL_TIMER_CC_1_INDEX




/* Defines for QEI_0 */
#define QEI_0_INST                                                         TIMG9
#define QEI_0_INST_IRQHandler                                   TIMG9_IRQHandler
#define QEI_0_INST_INT_IRQN                                     (TIMG9_INT_IRQn)
/* Pin configuration defines for QEI_0 PHA Pin */
#define GPIO_QEI_0_PHA_PORT                                                GPIOB
#define GPIO_QEI_0_PHA_PIN                                        DL_GPIO_PIN_29
#define GPIO_QEI_0_PHA_IOMUX                                     (IOMUX_PINCM66)
#define GPIO_QEI_0_PHA_IOMUX_FUNC                    IOMUX_PINCM66_PF_TIMG9_CCP0
/* Pin configuration defines for QEI_0 PHB Pin */
#define GPIO_QEI_0_PHB_PORT                                                GPIOB
#define GPIO_QEI_0_PHB_PIN                                        DL_GPIO_PIN_30
#define GPIO_QEI_0_PHB_IOMUX                                     (IOMUX_PINCM67)
#define GPIO_QEI_0_PHB_IOMUX_FUNC                    IOMUX_PINCM67_PF_TIMG9_CCP1

/* Defines for QEI_1 */
#define QEI_1_INST                                                         TIMG8
#define QEI_1_INST_IRQHandler                                   TIMG8_IRQHandler
#define QEI_1_INST_INT_IRQN                                     (TIMG8_INT_IRQn)
/* Pin configuration defines for QEI_1 PHA Pin */
#define GPIO_QEI_1_PHA_PORT                                                GPIOC
#define GPIO_QEI_1_PHA_PIN                                         DL_GPIO_PIN_6
#define GPIO_QEI_1_PHA_IOMUX                                     (IOMUX_PINCM84)
#define GPIO_QEI_1_PHA_IOMUX_FUNC                    IOMUX_PINCM84_PF_TIMG8_CCP0
/* Pin configuration defines for QEI_1 PHB Pin */
#define GPIO_QEI_1_PHB_PORT                                                GPIOA
#define GPIO_QEI_1_PHB_PIN                                        DL_GPIO_PIN_22
#define GPIO_QEI_1_PHB_IOMUX                                     (IOMUX_PINCM47)
#define GPIO_QEI_1_PHB_IOMUX_FUNC                    IOMUX_PINCM47_PF_TIMG8_CCP1


/* Defines for TIMER_7 */
#define TIMER_7_INST                                                     (TIMG7)
#define TIMER_7_INST_IRQHandler                                 TIMG7_IRQHandler
#define TIMER_7_INST_INT_IRQN                                   (TIMG7_INT_IRQn)
#define TIMER_7_INST_LOAD_VALUE                                         (19999U)
/* Defines for TIMER_12 */
#define TIMER_12_INST                                                   (TIMG12)
#define TIMER_12_INST_IRQHandler                               TIMG12_IRQHandler
#define TIMER_12_INST_INT_IRQN                                 (TIMG12_INT_IRQn)
#define TIMER_12_INST_LOAD_VALUE                                        (79999U)
/* Defines for TIMER_0 */
#define TIMER_0_INST                                                    (TIMG14)
#define TIMER_0_INST_IRQHandler                                TIMG14_IRQHandler
#define TIMER_0_INST_INT_IRQN                                  (TIMG14_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                         (49999U)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           40000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_0_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART1
#define UART_1_INST_FREQUENCY                                           40000000
#define UART_1_INST_IRQHandler                                  UART1_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOA
#define GPIO_UART_1_TX_PORT                                                GPIOA
#define GPIO_UART_1_RX_PIN                                         DL_GPIO_PIN_6
#define GPIO_UART_1_TX_PIN                                         DL_GPIO_PIN_5
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM11)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM10)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM11_PF_UART1_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM10_PF_UART1_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_1_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_4 */
#define UART_4_INST                                                        UART4
#define UART_4_INST_FREQUENCY                                           80000000
#define UART_4_INST_IRQHandler                                  UART4_IRQHandler
#define UART_4_INST_INT_IRQN                                      UART4_INT_IRQn
#define GPIO_UART_4_RX_PORT                                                GPIOB
#define GPIO_UART_4_TX_PORT                                                GPIOB
#define GPIO_UART_4_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_4_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_4_IOMUX_RX                                     (IOMUX_PINCM28)
#define GPIO_UART_4_IOMUX_TX                                     (IOMUX_PINCM27)
#define GPIO_UART_4_IOMUX_RX_FUNC                      IOMUX_PINCM28_PF_UART4_RX
#define GPIO_UART_4_IOMUX_TX_FUNC                      IOMUX_PINCM27_PF_UART4_TX
#define UART_4_BAUD_RATE                                                (115200)
#define UART_4_IBRD_80_MHZ_115200_BAUD                                      (43)
#define UART_4_FBRD_80_MHZ_115200_BAUD                                      (26)
/* Defines for UART_3 */
#define UART_3_INST                                                        UART3
#define UART_3_INST_FREQUENCY                                           80000000
#define UART_3_INST_IRQHandler                                  UART3_IRQHandler
#define UART_3_INST_INT_IRQN                                      UART3_INT_IRQn
#define GPIO_UART_3_RX_PORT                                                GPIOB
#define GPIO_UART_3_TX_PORT                                                GPIOB
#define GPIO_UART_3_RX_PIN                                        DL_GPIO_PIN_13
#define GPIO_UART_3_TX_PIN                                        DL_GPIO_PIN_12
#define GPIO_UART_3_IOMUX_RX                                     (IOMUX_PINCM30)
#define GPIO_UART_3_IOMUX_TX                                     (IOMUX_PINCM29)
#define GPIO_UART_3_IOMUX_RX_FUNC                      IOMUX_PINCM30_PF_UART3_RX
#define GPIO_UART_3_IOMUX_TX_FUNC                      IOMUX_PINCM29_PF_UART3_TX
#define UART_3_BAUD_RATE                                                (115200)
#define UART_3_IBRD_80_MHZ_115200_BAUD                                      (43)
#define UART_3_FBRD_80_MHZ_115200_BAUD                                      (26)




/* Defines for SPI_0 */
#define SPI_0_INST                                                         SPI0
#define SPI_0_INST_IRQHandler                                   SPI0_IRQHandler
#define SPI_0_INST_INT_IRQN                                       SPI0_INT_IRQn
#define GPIO_SPI_0_PICO_PORT                                              GPIOB
#define GPIO_SPI_0_PICO_PIN                                      DL_GPIO_PIN_17
#define GPIO_SPI_0_IOMUX_PICO                                   (IOMUX_PINCM43)
#define GPIO_SPI_0_IOMUX_PICO_FUNC                   IOMUX_PINCM43_PF_SPI0_PICO
/* GPIO configuration for SPI_0 */
#define GPIO_SPI_0_SCLK_PORT                                              GPIOB
#define GPIO_SPI_0_SCLK_PIN                                      DL_GPIO_PIN_18
#define GPIO_SPI_0_IOMUX_SCLK                                   (IOMUX_PINCM44)
#define GPIO_SPI_0_IOMUX_SCLK_FUNC                   IOMUX_PINCM44_PF_SPI0_SCLK
/* Defines for SPI_1 */
#define SPI_1_INST                                                         SPI1
#define SPI_1_INST_IRQHandler                                   SPI1_IRQHandler
#define SPI_1_INST_INT_IRQN                                       SPI1_INT_IRQn
#define GPIO_SPI_1_PICO_PORT                                              GPIOB
#define GPIO_SPI_1_PICO_PIN                                      DL_GPIO_PIN_15
#define GPIO_SPI_1_IOMUX_PICO                                   (IOMUX_PINCM32)
#define GPIO_SPI_1_IOMUX_PICO_FUNC                   IOMUX_PINCM32_PF_SPI1_PICO
#define GPIO_SPI_1_POCI_PORT                                              GPIOB
#define GPIO_SPI_1_POCI_PIN                                      DL_GPIO_PIN_14
#define GPIO_SPI_1_IOMUX_POCI                                   (IOMUX_PINCM31)
#define GPIO_SPI_1_IOMUX_POCI_FUNC                   IOMUX_PINCM31_PF_SPI1_POCI
/* GPIO configuration for SPI_1 */
#define GPIO_SPI_1_SCLK_PORT                                              GPIOB
#define GPIO_SPI_1_SCLK_PIN                                      DL_GPIO_PIN_16
#define GPIO_SPI_1_IOMUX_SCLK                                   (IOMUX_PINCM33)
#define GPIO_SPI_1_IOMUX_SCLK_FUNC                   IOMUX_PINCM33_PF_SPI1_SCLK



/* Defines for DMA_CH0 */
#define DMA_CH0_CHAN_ID                                                      (0)
#define UART_0_INST_DMA_TRIGGER_0                            (DMA_UART0_TX_TRIG)
/* Defines for DMA_CH1 */
#define DMA_CH1_CHAN_ID                                                      (1)
#define UART_0_INST_DMA_TRIGGER_1                            (DMA_UART0_RX_TRIG)
/* Defines for DMA_CH3 */
#define DMA_CH3_CHAN_ID                                                      (3)
#define UART_1_INST_DMA_TRIGGER_0                            (DMA_UART1_RX_TRIG)
/* Defines for DMA_CH4 */
#define DMA_CH4_CHAN_ID                                                      (4)
#define UART_1_INST_DMA_TRIGGER_1                            (DMA_UART1_TX_TRIG)
/* Defines for DMA_CH5 */
#define DMA_CH5_CHAN_ID                                                      (5)
#define UART_4_INST_DMA_TRIGGER_0                            (DMA_UART4_RX_TRIG)
/* Defines for DMA_CH2 */
#define DMA_CH2_CHAN_ID                                                      (2)
#define UART_4_INST_DMA_TRIGGER_1                            (DMA_UART4_TX_TRIG)
/* Defines for DMA_CH6 */
#define DMA_CH6_CHAN_ID                                                      (7)
#define UART_3_INST_DMA_TRIGGER_0                            (DMA_UART3_RX_TRIG)
/* Defines for DMA_CH7 */
#define DMA_CH7_CHAN_ID                                                      (6)
#define UART_3_INST_DMA_TRIGGER_1                            (DMA_UART3_TX_TRIG)


/* Port definition for Pin Group Key */
#define Key_PORT                                                         (GPIOB)

/* Defines for User: GPIOB.31 with pinCMx 68 on package pin 27 */
#define Key_User_PIN                                            (DL_GPIO_PIN_31)
#define Key_User_IOMUX                                           (IOMUX_PINCM68)
/* Defines for L1: GPIOB.19 with pinCMx 45 on package pin 60 */
#define LED_L1_PORT                                                      (GPIOB)
#define LED_L1_PIN                                              (DL_GPIO_PIN_19)
#define LED_L1_IOMUX                                             (IOMUX_PINCM45)
/* Defines for L2: GPIOC.7 with pinCMx 85 on package pin 64 */
#define LED_L2_PORT                                                      (GPIOC)
#define LED_L2_PIN                                               (DL_GPIO_PIN_7)
#define LED_L2_IOMUX                                             (IOMUX_PINCM85)
/* Defines for BLK: GPIOA.7 with pinCMx 14 on package pin 17 */
#define LCD_BLK_PORT                                                     (GPIOA)
#define LCD_BLK_PIN                                              (DL_GPIO_PIN_7)
#define LCD_BLK_IOMUX                                            (IOMUX_PINCM14)
/* Defines for DC: GPIOA.14 with pinCMx 36 on package pin 43 */
#define LCD_DC_PORT                                                      (GPIOA)
#define LCD_DC_PIN                                              (DL_GPIO_PIN_14)
#define LCD_DC_IOMUX                                             (IOMUX_PINCM36)
/* Defines for RES: GPIOC.0 with pinCMx 74 on package pin 46 */
#define LCD_RES_PORT                                                     (GPIOC)
#define LCD_RES_PIN                                              (DL_GPIO_PIN_0)
#define LCD_RES_IOMUX                                            (IOMUX_PINCM74)
/* Defines for PH1: GPIOA.31 with pinCMx 6 on package pin 7 */
#define Motor_PH1_PORT                                                   (GPIOA)
#define Motor_PH1_PIN                                           (DL_GPIO_PIN_31)
#define Motor_PH1_IOMUX                                           (IOMUX_PINCM6)
/* Defines for PH2: GPIOB.0 with pinCMx 12 on package pin 15 */
#define Motor_PH2_PORT                                                   (GPIOB)
#define Motor_PH2_PIN                                            (DL_GPIO_PIN_0)
#define Motor_PH2_IOMUX                                          (IOMUX_PINCM12)
/* Defines for nSLEEP1: GPIOB.4 with pinCMx 17 on package pin 20 */
#define Motor_nSLEEP1_PORT                                               (GPIOB)
#define Motor_nSLEEP1_PIN                                        (DL_GPIO_PIN_4)
#define Motor_nSLEEP1_IOMUX                                      (IOMUX_PINCM17)
/* Defines for nSLEEP2: GPIOB.5 with pinCMx 18 on package pin 21 */
#define Motor_nSLEEP2_PORT                                               (GPIOB)
#define Motor_nSLEEP2_PIN                                        (DL_GPIO_PIN_5)
#define Motor_nSLEEP2_IOMUX                                      (IOMUX_PINCM18)
/* Defines for nFault1: GPIOB.6 with pinCMx 23 on package pin 30 */
#define Motor_nFault1_PORT                                               (GPIOB)
#define Motor_nFault1_PIN                                        (DL_GPIO_PIN_6)
#define Motor_nFault1_IOMUX                                      (IOMUX_PINCM23)
/* Defines for nFault2: GPIOB.7 with pinCMx 24 on package pin 31 */
#define Motor_nFault2_PORT                                               (GPIOB)
#define Motor_nFault2_PIN                                        (DL_GPIO_PIN_7)
#define Motor_nFault2_IOMUX                                      (IOMUX_PINCM24)
/* Defines for IMU: GPIOC.4 with pinCMx 78 on package pin 52 */
#define Motor_IMU_PORT                                                   (GPIOC)
#define Motor_IMU_PIN                                            (DL_GPIO_PIN_4)
#define Motor_IMU_IOMUX                                          (IOMUX_PINCM78)
/* Defines for IN: GPIOC.9 with pinCMx 87 on package pin 66 */
#define Liner_IN_PORT                                                    (GPIOC)
#define Liner_IN_PIN                                             (DL_GPIO_PIN_9)
#define Liner_IN_IOMUX                                           (IOMUX_PINCM87)
/* Defines for OUT0: GPIOC.8 with pinCMx 86 on package pin 65 */
#define Liner_OUT0_PORT                                                  (GPIOC)
#define Liner_OUT0_PIN                                           (DL_GPIO_PIN_8)
#define Liner_OUT0_IOMUX                                         (IOMUX_PINCM86)
/* Defines for OUT1: GPIOB.26 with pinCMx 57 on package pin 76 */
#define Liner_OUT1_PORT                                                  (GPIOB)
#define Liner_OUT1_PIN                                          (DL_GPIO_PIN_26)
#define Liner_OUT1_IOMUX                                         (IOMUX_PINCM57)
/* Defines for OUT2: GPIOB.27 with pinCMx 58 on package pin 77 */
#define Liner_OUT2_PORT                                                  (GPIOB)
#define Liner_OUT2_PIN                                          (DL_GPIO_PIN_27)
#define Liner_OUT2_IOMUX                                         (IOMUX_PINCM58)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_PWMA_init(void);
void SYSCFG_DL_QEI_0_init(void);
void SYSCFG_DL_QEI_1_init(void);
void SYSCFG_DL_TIMER_7_init(void);
void SYSCFG_DL_TIMER_12_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_1_init(void);
void SYSCFG_DL_UART_4_init(void);
void SYSCFG_DL_UART_3_init(void);
void SYSCFG_DL_SPI_0_init(void);
void SYSCFG_DL_SPI_1_init(void);
void SYSCFG_DL_DMA_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
