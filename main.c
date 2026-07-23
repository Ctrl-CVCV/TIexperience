/*
 * Dual Motor Speed Control (PID @ 200Hz)
 * --- Tune parameters below ---
 */

#include "ti_msp_dl_config.h"
#include "BSP/Motor/motor.h"
#include "BSP/Encoder/encoder.h"
#include "BSP/Encoder/pid.h"
#include "BSP/UART_DMA/uart_dma.h"
#include "BSP/SPI0_OLED/spi0_oled.h"

/* ========== PID Parameters (TUNE HERE) ========== */
#define PID_A_KP   0.00f
#define PID_A_KI   0.00f
#define PID_A_KD   0.00f

#define PID_B_KP   0.001f
#define PID_B_KI   0.000f
#define PID_B_KD   0.00f

/* ========== Output Limit (30% duty = 300, motor rated 7.4V @ 12V supply) ========== */
#define PWM_LIMIT  300

/* ========== Speed Setpoints (Target RPM, motor shaft) ========== */
volatile int16_t g_setpoint_a = 1000;   /* Motor A target RPM */
volatile int16_t g_setpoint_b = 1000;   /* Motor B target RPM */

/* ========== Internals ========== */
volatile uint32_t nowtime;              /* IMU.o reference */
volatile bool     g_ctrl_tick = false;  /* set by TIMER_7 ISR, consumed by main loop */
static PID_Controller g_pid_a, g_pid_b;
static float g_pwm_a, g_pwm_b;   /* float accumulation, no truncation */
static char   g_msg[80];
static const float g_dt = 0.005f;       /* 200Hz = 5ms */

/* ---- itoa helper (no padding) ---- */
static char *itoa(char *dst, int16_t val)
{
    uint16_t u; char tmp[8]; int i = 0, j;
    if (val < 0) { *dst++ = '-'; u = (uint16_t)(-val); }
    else         { u = (uint16_t)val; }
    if (u == 0) tmp[i++] = '0';
    while (u) { tmp[i++] = (char)('0' + (u % 10)); u /= 10; }
    for (j = i - 1; j >= 0; j--) *dst++ = tmp[j];
    return dst;
}

/* ---- TIMER_7 ISR: only set flag, no heavy work ---- */
void TIMER_7_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_7_INST)) {
    case DL_TIMER_IIDX_ZERO:
        DL_TimerG_clearInterruptStatus(TIMER_7_INST,
            DL_TIMERG_INTERRUPT_ZERO_EVENT);
        g_ctrl_tick = true;
        break;
    default: break;
    }
}

/* ---- Main ---- */
int main(void)
{
    int16_t rpm_a, rpm_b;
    char   *p;

    SYSCFG_DL_init();
    motor_init();
    encoder_init();
    uart0_dma_init();
    uart4_dma_init();

    /* PB11 (UART4 RX) 改为 GPIO 输入上拉，防止浮空噪声灌满 RX FIFO */
    DL_GPIO_initDigitalInput(GPIO_UART_4_IOMUX_RX);
    DL_GPIO_initDigitalInputFeatures(GPIO_UART_4_IOMUX_RX,
             DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
             DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* OLED */
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (u8 *)"OLED OK");
    OLED_ShowString(0, 2, (u8 *)"U4 DMA ready");

    /* Init PIDs */
    PID_Init(&g_pid_a, PID_A_KP, PID_A_KI, PID_A_KD, (float)PWM_LIMIT);
    PID_Init(&g_pid_b, PID_B_KP, PID_B_KI, PID_B_KD, (float)PWM_LIMIT);

    /* TIMER_7: 200Hz */
    {
        DL_TimerG_TimerConfig cfg = {
            .period    = 49999,
            .timerMode = DL_TIMER_TIMER_MODE_PERIODIC,
            .startTimer = DL_TIMER_START,
        };
        DL_TimerG_initTimerMode(TIMER_7_INST, &cfg);
        DL_TimerG_enableInterrupt(TIMER_7_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
        NVIC_SetPriority(TIMER_7_INST_INT_IRQN, 2);
        NVIC_EnableIRQ(TIMER_7_INST_INT_IRQN);
    }

    while (1) {
        while (!g_ctrl_tick) { /* spin */ }
        g_ctrl_tick = false;

        rpm_a = encoder_b_get_rpm_filt();
        rpm_b = encoder_a_get_rpm_filt();

        g_pwm_a = 200;
        g_pwm_b = 200;
        motor_a_run((int16_t)g_pwm_a);
        motor_b_run((int16_t)g_pwm_b);

        /* UART4 DMA: rpm_a,rpm_b (PB10, 硬件 UART4) */
        if (!uart4_dma_is_busy()) {
            p = g_msg;
            p = itoa(p, rpm_a);
            *p++ = ',';
            p = itoa(p, rpm_b);
            *p++ = '\n';
            uart4_dma_send((const uint8_t *)g_msg, (uint16_t)(p - g_msg));
        }

        /* OLED: 显示 RPM */
        {
            char buf[16];
            OLED_ShowString(0, 4, (u8 *)"A:");
            p = itoa(buf, rpm_a);
            *p = '\0';
            OLED_ShowString(16, 4, (u8 *)buf);
            OLED_ShowString(0, 6, (u8 *)"B:");
            p = itoa(buf, rpm_b);
            *p = '\0';
            OLED_ShowString(16, 6, (u8 *)buf);
        }
    }
}
