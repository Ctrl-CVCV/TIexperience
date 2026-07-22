/*
 * Dual Motor Speed Control (PID @ 200Hz)
 * --- Tune parameters below ---
 */

#include "ti_msp_dl_config.h"
#include "BSP/Motor/motor.h"
#include "BSP/Encoder/encoder.h"
#include "BSP/Encoder/pid.h"
#include "BSP/UART_DMA/uart_dma.h"

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

    /* Init PIDs — output clamped to 30% duty */
    PID_Init(&g_pid_a, PID_A_KP, PID_A_KI, PID_A_KD, (float)PWM_LIMIT);
    PID_Init(&g_pid_b, PID_B_KP, PID_B_KI, PID_B_KD, (float)PWM_LIMIT);

    /* Set up TIMER_7 for periodic 200Hz (5ms)
     * TIMER_7 clk = 80MHz/8 = 10MHz. period = 0.005*10M-1 = 49999 */
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
        /* Wait for next 200Hz tick */
        while (!g_ctrl_tick) { /* spin */ }
        g_ctrl_tick = false;

        rpm_a = encoder_b_get_rpm_filt();  /* QEI_1 → Motor A */
        rpm_b = encoder_a_get_rpm_filt();  /* QEI_0 → Motor B */

        /* Open-loop PWM — no PID */
        g_pwm_a = 200;
        g_pwm_b = 200;

        motor_a_run((int16_t)g_pwm_a);
        motor_b_run((int16_t)g_pwm_b);

        /* UART: rpm_a,rpm_b */
        if (!uart0_dma_is_busy()) {
            p = g_msg;
            p = itoa(p, rpm_a);
            *p++ = ',';
            p = itoa(p, rpm_b);
            *p++ = '\n';
            uart0_dma_send((const uint8_t *)g_msg, (uint16_t)(p - g_msg));
        }
    }
}
