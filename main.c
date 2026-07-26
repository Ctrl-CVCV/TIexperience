/*
 * Dual Motor PID Speed Control
 * --- 500Hz TIMER_7 interrupt drives PID loop ---
 */

#include "ti_msp_dl_config.h"
#include "BSP/Motor/motor.h"
#include "BSP/Encoder/encoder.h"
#include "pid.h"
#include "BSP/UART_DMA/uart_dma.h"
#include "BSP/SPI0_LCD/lcd.h"

/* ========== PID Parameters (TUNE HERE) ========== */
#define PID_A_KP   0.30f
#define PID_A_KI   0.00f
#define PID_A_KD   0.00f

#define PID_B_KP   0.30f
#define PID_B_KI   0.00f
#define PID_B_KD   0.00f

/* ========== Output Limit (50% duty cycle = 500) ========== */
#define PWM_LIMIT  500

/* ========== Speed Setpoints ========== */
volatile int16_t g_setpoint_a = 1500;
volatile int16_t g_setpoint_b = 1500;

/* ========== Internals ========== */
volatile uint32_t nowtime;
volatile bool     g_ctrl_tick = false;
static PID_Controller pid_rpma, pid_rpmb;
static float g_pwm_a, g_pwm_b = 0.0f;
float g_pwm_af, g_pwm_bf = 0.0f;
static char   g_msg[80];
static const float g_dt = 0.002f;       /* 500Hz = 2ms */

/* ---- itoa helper ---- */
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

/* ---- TIMER_7 ISR: 500Hz flag ---- */
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
    encoder_init();
    uart0_dma_init();
    uart4_dma_init();
    uart3_dma_init();
    /* PB11 (UART4 RX) as GPIO input pull-up, prevent floating noise */
    DL_GPIO_initDigitalInput(GPIO_UART_4_IOMUX_RX);
    DL_GPIO_initDigitalInputFeatures(GPIO_UART_4_IOMUX_RX,
             DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
             DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    /* PID Init */
    PID_Init(&pid_rpma, PID_A_KP, PID_A_KI, PID_A_KD, (float)PWM_LIMIT);
    PID_Init(&pid_rpmb, PID_B_KP, PID_B_KI, PID_B_KD, (float)PWM_LIMIT);
    /* LCD init BEFORE motor to confirm SPI works */
    LCD_Init();
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_ShowString(0, 0, (const u8 *)"LCD OK", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, 22, (const u8 *)"Motor PID", WHITE, BLACK, 16, 0);

    /* Motor init AFTER LCD */
    motor_init();

    /* TIMER_7: 500Hz */
    {
        DL_TimerG_stopCounter(TIMER_7_INST);
        DL_TimerG_clearInterruptStatus(TIMER_7_INST,
            DL_TIMERG_INTERRUPT_ZERO_EVENT);
        DL_TimerG_TimerConfig cfg = {
            .period    = TIMER_7_INST_LOAD_VALUE,
            .timerMode = DL_TIMER_TIMER_MODE_PERIODIC,
            .startTimer = DL_TIMER_START,
        };
        DL_TimerG_initTimerMode(TIMER_7_INST, &cfg);
        DL_TimerG_enableInterrupt(TIMER_7_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
        NVIC_SetPriority(TIMER_7_INST_INT_IRQN, 2);
        NVIC_EnableIRQ(TIMER_7_INST_INT_IRQN);
    }

    LCD_ShowString(0, 44, (const u8 *)"PID Speed", WHITE, BLACK, 16, 0);

    while (1) {
        /* Wait for 500Hz tick, keep reading fresh RPMs */
        while (!g_ctrl_tick) {
            rpm_a = encoder_a_get_rpm_filt();
            rpm_b = encoder_b_get_rpm_filt();
        }
        g_ctrl_tick = false;

        /* PID control — once per tick (500Hz), result = absolute PWM */
        g_pwm_af += PID_Update(&pid_rpma, (float)g_setpoint_a, (float)rpm_a, g_dt);
        g_pwm_bf += PID_Update(&pid_rpmb, (float)g_setpoint_b, (float)rpm_b, g_dt);
        if (g_pwm_af > PWM_LIMIT)
            g_pwm_af = PWM_LIMIT;
        if (g_pwm_af < -PWM_LIMIT)
            g_pwm_af = -PWM_LIMIT;
        if (g_pwm_bf > PWM_LIMIT)
            g_pwm_bf = PWM_LIMIT;
        if (g_pwm_bf < -PWM_LIMIT)
            g_pwm_bf = -PWM_LIMIT;
        g_pwm_a = (int16_t)g_pwm_af;
        g_pwm_b = (int16_t)g_pwm_bf;
        motor_a_run(g_pwm_a);
        motor_b_run(g_pwm_b);

        /* UART telemetry: pwm_a,pwm_b,rpm_a,rpm_b */
        if (!uart3_dma_is_busy()) {
            p = g_msg;
            p = itoa(p, g_pwm_a);
            *p++ = ',';
            p = itoa(p, g_pwm_b);
            *p++ = ',';
            p = itoa(p, rpm_a);
            *p++ = ',';
            p = itoa(p, rpm_b);
            *p++ = '\n';
            uart3_dma_send((const uint8_t *)g_msg, (uint16_t)(p - g_msg));
        }

        /* LCD: show RPM */
        {
            char buf[16];
            p = itoa(buf, rpm_a); *p = '\0';
            LCD_ShowString(0, 66, (const u8 *)"A rpm:", WHITE, BLACK, 16, 0);
            LCD_Fill(80, 66, 180, 82, BLACK);
            LCD_ShowString(80, 66, (const u8 *)buf, WHITE, BLACK, 16, 0);
            p = itoa(buf, rpm_b); *p = '\0';
            LCD_ShowString(0, 88, (const u8 *)"B rpm:", WHITE, BLACK, 16, 0);
            LCD_Fill(80, 88, 180, 104, BLACK);
            LCD_ShowString(80, 88, (const u8 *)buf, WHITE, BLACK, 16, 0);
            p = itoa(buf, (int16_t)encoder_get_isr_count()); *p = '\0';
            LCD_ShowString(0, 110, (const u8 *)"ISR:", WHITE, BLACK, 16, 0);
            LCD_Fill(40, 110, 100, 126, BLACK);
            LCD_ShowString(40, 110, (const u8 *)buf, WHITE, BLACK, 16, 0);
        }
    }
}
