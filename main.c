/*
 * DRV8874 Motor Speed Control (PID @ 200Hz)
 * --- Tune parameters below ---
 */

#include "ti_msp_dl_config.h"
#include "BSP/Motor/motor.h"
#include "BSP/Encoder/encoder.h"
#include "BSP/Encoder/pid.h"
#include "BSP/UART_DMA/uart_dma.h"

/* ========== PID Parameters (TUNE HERE) ========== */
#define PID_KP   0.50f
#define PID_KI   0.26f
#define PID_KD   0.00f

/* ========== Speed Setpoint (Target RPM, motor shaft) ========== */
volatile int16_t g_setpoint = 2000;

/* ========== Internals ========== */
volatile uint32_t nowtime;              /* referenced by pid.c */
volatile bool     g_ctrl_tick = false;
static PID_Controller g_pid;
static float g_pwm;
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
    int16_t rpm;
    char   *p;

    SYSCFG_DL_init();
    motor_init();
    encoder_init();
    uart_dma_init();

    /* Init PID */
    PID_Init(&g_pid, PID_KP, PID_KI, PID_KD, 600.0f);

    /* Set up TIMER_7 for periodic 200Hz */
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

        rpm = encoder_a_get_rpm_filt();  /* QEI_0 */

        /* ---- Fixed PWM test — change value here ---- */
        g_pwm = 100;   /* 10% duty */
        /* ---- PID mode — uncomment below and comment above ---- */
        // g_pwm = PID_Update(&g_pid, (float)g_setpoint, (float)rpm, g_dt);

        motor_run((int16_t)g_pwm);

        /* UART: rpm only */
        if (!uart_dma_is_busy()) {
            p = g_msg;
            p = itoa(p, rpm);
            *p++ = '\n';
            uart_dma_send((const uint8_t *)g_msg, (uint16_t)(p - g_msg));
        }
    }
}
