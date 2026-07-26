#ifndef SPI1_H
#define SPI1_H

/*
 * SPI1 模块头文件。
 *
 * 职责：
 * - 提供 SPI1 单字节全双工读写接口，供 IMU 寄存器驱动发送地址、数据和 dummy 字节。
 * - 提供带成功/失败返回值的传输接口，便于初始化阶段把 SPI 超时上报给芯片驱动。
 * - 提供 IMU 片选控制宏，片选引脚由 SysConfig 生成的 IMU_PORT/IMU_CS_PIN 描述。
 *
 * 硬件约束：
 * - SPI1 的 SCLK/MOSI/MISO 当前复用到 PB16/PB15/PB14。
 * - IMU CS 是普通 GPIO，空闲必须保持高电平，完整寄存器事务期间必须保持低电平。
 * - 本头文件属于 BSP 模块头文件，只依赖 bsp_common.h，不包含聚合头 bsp.h。
 */
#include "bsp_common.h"

bool spi1_transfer_byte(uint8_t tx_data, uint8_t *rx_data);

uint8_t spi1_read_write_byte(uint8_t dat);

/*
 * IMU 片选控制。
 *
 * 输入：
 * - level 非 0：释放 IMU 片选，CS 输出高电平。
 * - level 为 0：选中 IMU，CS 输出低电平。
 *
 * 说明：
 * - LSM6DSV SPI 事务以 CS 低电平窗口为边界，读写地址字节和连续数据字节必须在同一窗口内完成。
 */
#define SPI1_CS_IMU(level)                                                    \
    do {                                                                      \
        if ((level) != 0U) {                                                   \
            DL_GPIO_setPins(IMU_PORT, IMU_CS_PIN);                             \
        } else {                                                              \
            DL_GPIO_clearPins(IMU_PORT, IMU_CS_PIN);                           \
        }                                                                     \
    } while (0)

#endif
