/**
  ******************************************************************************
  * @file    uart_task.c
  * @brief   UART7 命令处理任务
  ******************************************************************************
  */
#include "uart_task.h"
#include "uart_bsp.h"
#include "imu_task.h"
#include "usart.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern uint8_t calib_mode;
extern uint8_t motor_enabled;

void UartDemoTask_Entry(void const * argument)
{
    static uart7_rx_frame_t frame;
    char tx_buf[128];
    int tx_len;

    uart7_rx_init();

    tx_len = snprintf(tx_buf, sizeof(tx_buf),
        "\r\n=== UART7 READY ===\r\n"
        "Commands: CAL, HELP\r\n");
    uart7_send_dma((uint8_t*)tx_buf, tx_len);
    while (huart7.gState == HAL_UART_STATE_BUSY_TX) { osDelay(1); }

    for (;;)
    {
        if (xQueueReceive(uart7_rx_queue, &frame, portMAX_DELAY) == pdTRUE)
        {
            if (frame.len < sizeof(frame.data))
                frame.data[frame.len] = '\0';
            else
                frame.data[sizeof(frame.data) - 1] = '\0';

            char *cmd = (char*)frame.data;

            for (int i = (int)frame.len - 1; i >= 0; i--)
            {
                if (frame.data[i] == '\r' || frame.data[i] == '\n')
                    frame.data[i] = '\0';
                else
                    break;
            }

            if (strcmp(cmd, "CAL") == 0 || strcmp(cmd, "cal") == 0)
            {
                tx_len = snprintf(tx_buf, sizeof(tx_buf),
                    "ANGLE:%.2f\r\n", g_shaft_angle);
                uart7_send_dma((uint8_t*)tx_buf, tx_len);
            }
            else if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "help") == 0)
            {
                tx_len = snprintf(tx_buf, sizeof(tx_buf),
                    "Commands:\r\n"
                    "  CAL  - report current shaft angle\r\n"
                    "  HELP - show this help\r\n");
                uart7_send_dma((uint8_t*)tx_buf, tx_len);
            }
            else if (frame.len > 0)
            {
                tx_len = snprintf(tx_buf, sizeof(tx_buf),
                    "UNKNOWN: '%s'. Type HELP for commands.\r\n", cmd);
                uart7_send_dma((uint8_t*)tx_buf, tx_len);
            }

            while (huart7.gState == HAL_UART_STATE_BUSY_TX) { osDelay(1); }
        }
    }
}
