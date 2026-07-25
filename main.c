/*
 * Dual Motor Direction & LR Identification Test
 * --- One motor at a time, cycles through FWD/STOP/REV/STOP ---
 */

#include "ti_msp_dl_config.h"
#include "BSP/Motor/motor.h"
#include "BSP/Encoder/encoder.h"
#include "BSP/Encoder/pid.h"
#include "BSP/UART_DMA/uart_dma.h"
#include "BSP/SPI0_LCD/lcd.h"

/* ========== PID Parameters (TUNE HERE) ========== */
#define PID_A_KP   0.00f
#define PID_A_KI   0.00f
#define PID_A_KD   0.00f

#define PID_B_KP   0.001f
#define PID_B_KI   0.000f
#define PID_B_KD   0.00f

/* ========== Output Limit (30% duty = 300) ========== */
#define PWM_LIMIT  300

/* ========== Speed Setpoints ========== */
volatile int16_t g_setpoint_a = 1000;
volatile int16_t g_setpoint_b = 1000;

/* ========== Internals ========== */
volatile uint32_t nowtime;
volatile bool     g_ctrl_tick = false;
static PID_Controller g_pid_a, g_pid_b;
static float g_pwm_a, g_pwm_b;
static char   g_msg[80];
static const float g_dt = 0.005f;       /* 200Hz = 5ms */

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

/* ---- TIMER_7 ISR: 200Hz flag ---- */
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

    /* LCD init BEFORE motor to confirm SPI works */
    LCD_Init();
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_ShowString(0, 0, (const u8 *)"LCD OK", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, 22, (const u8 *)"Motor Test", WHITE, BLACK, 16, 0);

    /* Motor init AFTER LCD */
    motor_init();

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

    /* ---- Motor direction & LR identify test ---- */
    #define TEST_PWM      250    /* test duty (25%) */
    #define T_ON          400    /* 2s @ 200Hz */
    #define T_OFF         200    /* 1s @ 200Hz */
    enum { S_A_FWD, S_A_STOP, S_A_REV, S_A_STOP2,
           S_B_FWD, S_B_STOP, S_B_REV, S_B_STOP2 };
    uint8_t  state = S_A_FWD;
    uint16_t tick  = 0;
    int16_t  pwm_a = 0, pwm_b = 0;

    LCD_ShowString(0, 44, (const u8 *)"Dir Test", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, 66, (const u8 *)"A FWD  ", WHITE, BLACK, 16, 0);

    while (1) {
        /* Wait for 200Hz tick */
        while (!g_ctrl_tick) { /* spin */ }
        g_ctrl_tick = false;

        rpm_a = encoder_a_get_rpm_filt();
        rpm_b = encoder_b_get_rpm_filt();

        /* State transition on timer */
        if (++tick >= (state & 1 ? T_OFF : T_ON)) {
            tick = 0;
            state = (state + 1) & 7;  /* wrap 0..7 */
            motor_a_run(0);
            motor_b_run(0);
        }

        switch (state) {
        case S_A_FWD:   pwm_a =  TEST_PWM; pwm_b =  0;         break;
        case S_A_STOP:  pwm_a =  0;        pwm_b =  0;         break;
        case S_A_REV:   pwm_a = -TEST_PWM; pwm_b =  0;         break;
        case S_A_STOP2: pwm_a =  0;        pwm_b =  0;         break;
        case S_B_FWD:   pwm_a =  0;        pwm_b =  TEST_PWM;  break;
        case S_B_STOP:  pwm_a =  0;        pwm_b =  0;         break;
        case S_B_REV:   pwm_a =  0;        pwm_b = -TEST_PWM;  break;
        case S_B_STOP2: pwm_a =  0;        pwm_b =  0;         break;
        }
        motor_a_run(pwm_a);
        motor_b_run(pwm_b);

        /* UART: state,tick,pwm_a,pwm_b,rpm_a,rpm_b */
        if (!uart4_dma_is_busy()) {
            p = g_msg;
            *p++ = '0' + state; *p++ = ',';
            p = itoa(p, tick); *p++ = ',';
            p = itoa(p, pwm_a); *p++ = ',';
            p = itoa(p, pwm_b); *p++ = ',';
            p = itoa(p, rpm_a); *p++ = ',';
            p = itoa(p, rpm_b); *p++ = '\n';
            uart4_dma_send((const uint8_t *)g_msg, (uint16_t)(p - g_msg));
        }

        /* LCD: show state + RPM */
        {
            static const char *labels[8] = {
                "A FWD  ", "A STOP ", "A REV  ", "A STOP ",
                "B FWD  ", "B STOP ", "B REV  ", "B STOP "};
            char buf[16];

            LCD_ShowString(0, 66, (const u8 *)labels[state], WHITE, BLACK, 16, 0);

            /* A rpm */
            p = itoa(buf, rpm_a); *p = '\0';
            LCD_ShowString(0, 90, (const u8 *)"A rpm:", WHITE, BLACK, 16, 0);
            LCD_Fill(80, 90, 180, 106, BLACK);  /* erase old number */
            LCD_ShowString(80, 90, (const u8 *)buf, WHITE, BLACK, 16, 0);

            /* B rpm */
            p = itoa(buf, rpm_b); *p = '\0';
            LCD_ShowString(0, 112, (const u8 *)"B rpm:", WHITE, BLACK, 16, 0);
            LCD_Fill(80, 112, 180, 128, BLACK);
            LCD_ShowString(80, 112, (const u8 *)buf, WHITE, BLACK, 16, 0);
        }
    }
}
