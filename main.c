/*
 * Dual Motor PID Speed Control
 * --- 500Hz TIMER_7 interrupt drives PID loop ---
 */

#include "ti_msp_dl_config.h"
#include "BSP/Motor/motor.h"
#include "BSP/Encoder/encoder.h"
#include "pid.h"
#include "BSP/UART_DMA/uart_dma.h"
#include "IMU/IMU.h"              /* before LCD: brings bsp_common.h → BSP_DELAY_MS_DEFINED */
#include "BSP/SPI0_LCD/lcd.h"
#include "BSP/liner/liner.h"
#include "BSP/Servo/servo.h"
/* ========== PID Parameters (TUNE HERE) ========== */
#define PID_A_KP   0.05f
#define PID_A_KI   0.035f
#define PID_A_KD   0.0f

#define PID_B_KP   0.05f
#define PID_B_KI   0.035f
#define PID_B_KD   0.00f

/* ========== Output Limit (50% duty cycle = 500) ========== */
#define PWM_LIMIT  500

/* ========== Liner PID (line-position PD) ========== */
#define LINER_PID_KP   0.8f
#define LINER_PID_KI   0.0f
#define LINER_PID_KD   0.0f

/* ========== Internals ========== */
volatile uint32_t nowtime;
volatile bool     g_ctrl_tick = false;
static PID_Controller pid_rpma, pid_rpmb, pid_liner;
static float g_pwm_a, g_pwm_b = 0.0f;
float g_pwm_af, g_pwm_bf = 0.0f;
static char   g_msg[80];
static const float g_dt = 0.002f;       /* 500Hz = 2ms */
float g_imu_ypr[3];                     /* IMU yaw/pitch/roll updated @ 100Hz */
static bool g_motor_enabled = false;     /* PA17 button to start */

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

/* ---- ftoa1: float → string with 1 decimal place ---- */
static char *ftoa1(char *dst, float val)
{
    int32_t scaled;
    if (val < 0.0f) {
        *dst++ = '-';
        scaled = (int32_t)(-val * 10.0f + 0.5f);
    } else {
        scaled = (int32_t)(val * 10.0f + 0.5f);
    }
    dst = itoa(dst, (int16_t)(scaled / 10));
    *dst++ = '.';
    *dst++ = (char)('0' + (scaled % 10));
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
    servo_init();
     /* IMU init */
    IMU_init();

    /* Motor init AFTER LCD */
    motor_init();
    motor_a_run(0);
    motor_b_run(0);

    /* Liner init: ISR computes target_speed_rpm, main loop PID tracks */
    liner_init();
    PID_Init(&pid_liner, LINER_PID_KP, LINER_PID_KI, LINER_PID_KD, 1.0f);
    liner_set_pid(&pid_liner);
    liner_resume();  /* enable liner_control2 in ISR */
    liner_start();   /* TIMG14 @ 200Hz */
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

    static bool imu_pending = false;

    while (1) {
        /* Idle: do background work while waiting for 500Hz tick */
        while (!g_ctrl_tick) {
            if (imu_pending) {
                imu_pending = false;
                IMU_getYawPitchRoll(g_imu_ypr);
            }
        }

        /* --- Critical path: encoder → PID → motor (must be < 2ms) --- */
        rpm_a = encoder_a_get_rpm();
        rpm_b = encoder_b_get_rpm();
        g_ctrl_tick = false;
        /* PA17 button: press to start motors (debounce 50ms) */
        if (!g_motor_enabled) {
            static uint8_t btn_cnt;
            if (DL_GPIO_readPins(GPIOA, Key_User_PIN) == 0U) {
                if (++btn_cnt >= 25) {
                    g_motor_enabled = true;
                    PID_Init(&pid_rpma, PID_A_KP, PID_A_KI, PID_A_KD, (float)PWM_LIMIT);
                    PID_Init(&pid_rpmb, PID_B_KP, PID_B_KI, PID_B_KD, (float)PWM_LIMIT);
                    g_pwm_af = 0.0f;
                    g_pwm_bf = 0.0f;
                }
            } else {
                btn_cnt = 0;
            }
        }

        if (g_motor_enabled) {
            /* PID speed control — track liner target RPM */
            g_pwm_af += PID_Update(&pid_rpma, target_speed_rpm1, (float)rpm_a, g_dt);
            g_pwm_bf += PID_Update(&pid_rpmb, target_speed_rpm2, (float)rpm_b, g_dt);
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
        } else {
            g_pwm_a = 0;
            g_pwm_b = 0;
            motor_a_run(0);
            motor_b_run(0);
        }

        /* Flag IMU read for idle period (100Hz = every 5 ticks) */
        {
            static uint8_t imu_tick;
            if (++imu_tick >= 5) {
                imu_tick = 0;
                imu_pending = true;
            }
        }

        /* UART telemetry: pwm_a,pwm_b,rpm_a,rpm_b,yaw */
        if (!uart3_dma_is_busy()) {
            p = g_msg;
            p = itoa(p, g_pwm_a);
            *p++ = ',';
            p = itoa(p, g_pwm_b);
            *p++ = ',';
            p = itoa(p, rpm_a);
            *p++ = ',';
            p = itoa(p, rpm_b);
            *p++ = ',';
            p = ftoa1(p, g_imu_ypr[0]);
            *p++ = '\n';
            uart3_dma_send((const uint8_t *)g_msg, (uint16_t)(p - g_msg));
        }
#if 1
        /* LCD: all display updates @ 20Hz (every 25 ticks) */
        {
            static uint8_t lcd_tick;
            if (++lcd_tick >= 25) {
                lcd_tick = 0;
                char buf[20];

                /* IMU status */
                {
                    const char *imu_str;
                    uint16_t    imu_color;
                    if (!IMU_isReady()) {
                        imu_str = "IMU FAIL"; imu_color = RED;
                    } else if (!IMU_isCalibrated()) {
                        imu_str = "IMU CAL";  imu_color = YELLOW;
                    } else {
                        imu_str = "IMU OK";   imu_color = GREEN;
                    }
                    LCD_ShowString(120, 44, (const u8 *)imu_str, imu_color, BLACK, 16, 0);
                }

                /* --- Encoder debug --- */
                p = itoa(buf, rpm_a); *p++ = ' '; p = itoa(p, rpm_b); *p = '\0';
                LCD_ShowString(0, 54, (const u8 *)"RPM:", WHITE, BLACK, 16, 0);
                LCD_Fill(48, 54, 160, 70, BLACK);
                LCD_ShowString(48, 54, (const u8 *)buf, WHITE, BLACK, 16, 0);

                p = itoa(buf, (int16_t)target_speed_rpm1); *p++ = ' ';
                p = itoa(p, (int16_t)target_speed_rpm2); *p = '\0';
                LCD_ShowString(0, 72, (const u8 *)"TGT:", WHITE, BLACK, 16, 0);
                LCD_Fill(48, 72, 160, 88, BLACK);
                LCD_ShowString(48, 72, (const u8 *)buf, WHITE, BLACK, 16, 0);


                /* --- Liner sensor states --- */
                LCD_ShowString(0, 130, (const u8 *)"LR:", WHITE, BLACK, 16, 0);
                for (int i = 0; i < 8; i++) {
                    /* display: 1=line(green), 0=no-line(gray) �?inverted vs raw */
                    char ch = liner_states[i] ? '0' : '1';
                    LCD_ShowChar(24 + (u16)i * 12, 130, (u8)ch,
                                 liner_states[i] ? GRAY : GREEN, BLACK, 12, 0);
                }
                /* Filtered position */
                {
                    char dbuf[12];
                    int16_t ival = (int16_t)(ave_values_after_filtered * 1000.0f);
                    char *dp2 = itoa(dbuf, ival);
                    *dp2 = '\0';
                    LCD_Fill(0, 148, 160, 164, BLACK);
                    LCD_ShowString(0, 148, (const u8 *)dbuf, WHITE, BLACK, 16, 0);
                }
                if (is_lost_line) {
                    LCD_ShowString(120, 148, (const u8 *)"LOST", RED, BLACK, 16, 0);
                }
                /* --- IMU YAW --- */
                {
                    char ybuf[12];
                    int16_t yaw_i = (int16_t)g_imu_ypr[0];
                    char *yp = itoa(ybuf, yaw_i);
                    *yp = '\0';
                    LCD_Fill(0, 166, 160, 182, BLACK);
                    LCD_ShowString(0, 166, (const u8 *)"YAW:", WHITE, BLACK, 16, 0);
                    LCD_ShowString(48, 166, (const u8 *)ybuf, WHITE, BLACK, 16, 0);
                }
            }
        }
#endif
    }
}
