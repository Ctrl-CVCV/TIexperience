#ifndef BSP_COMMON_H
#define BSP_COMMON_H

/*
 * BSP 公共基础头文件。
 *
 * 职责：
 * 1. 统一收口 C 标准库、MSPM0 DriverLib、CMSIS 内核接口和 SysConfig 生成的硬件宏。
 * 2. 作为 BSP 各模块头文件的公共依赖，避免模块头文件反向包含聚合头 bsp.h。
 * 3. 统一声明跨模块都会用到的基础接口，例如 UART0 中实现的阻塞延时函数。
 *
 * 约束：
 * - 本文件只能放公共基础能力，不能把某个具体 BSP 外设模块的业务接口放进来。
 * - 新增 BSP 模块头文件应默认包含本文件，而不是直接包含 bsp.h。
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "ti_msp_dl_config.h"

/*
 * UART_WF (UART4) — 未在 SysConfig 中配置，手动提供宏定义。
 *
 * 这些符号被 wb2_wifi.c 和 peripheral_test.c 引用。
 * 放在这里而非 ti_msp_dl_config.h 是为了避免 SysConfig 预构建覆盖。
 */
#define UART_WF_INST            UART4
#define UART_WF_BAUD_RATE       (115200U)

/*
 * 毫秒级阻塞延时。
 *
 * 参数：
 * - ms：需要阻塞等待的毫秒数。
 *
 * 实现位置：
 * - 由 BSP/UART0/uart0.c 提供，公共声明放在这里便于 SPI、IMU、LCD 等底层模块使用。
 *
 * 使用约束：
 * - 该函数为忙等阻塞，不应在中断服务函数中调用。
 */
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

/*
 * IMU 引脚名映射 — SysConfig 生成的宏名为 Motor_IMU_*，
 * LSM6DSV 驱动代码内部使用的是 IMU_PORT / IMU_CS_PIN。
 */
#define IMU_PORT    Motor_IMU_PORT
#define IMU_CS_PIN  Motor_IMU_PIN

#endif
