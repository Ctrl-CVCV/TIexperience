/*
 * motor.c - DRV8874 single motor control (Channel 2)
 *
 * EN/IN1  = TIMG6 CH1 (PA30)  — PWM speed
 * PH/IN2  = PB0               — direction (HIGH=fwd, LOW=rev)
 * nSLEEP  = PB5               — HIGH=enable, LOW=sleep
 */

#include "motor.h"
#include "../ti_msp_dl_config.h"

#define MOTOR_PWM_INST    MOTOR1_PWM_INST          /* TIMG6 */
#define MOTOR_PWM_IDX     DL_TIMER_CC_1_INDEX       /* CH1 = PA30 = EN2 */
#define MOTOR_PWM_PERIOD  1000                       /* matches SysConfig, 80kHz */

/* PH2  = PB0 = IOMUX_PINCM13 (not in SysConfig) */
/* nSLEEP2 = PB5 = IOMUX_PINCM18 (not in SysConfig) */
#define MOTOR_PH2_IOMUX     (IOMUX_PINCM13)
#define MOTOR_nSLEEP2_IOMUX (IOMUX_PINCM18)

void motor_init(void)
{
    /* nSLEEP2 = PB5. DRV8874 needs nSLEEP rising edge to wake up. */
    DL_GPIO_initDigitalOutput(MOTOR_nSLEEP2_IOMUX);
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_5);
    for (volatile int i = 0; i < 1000; i++) { __NOP(); }
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_5);     /* nSLEEP = HIGH → wake up */

    /* PH2 = PB0, init as output LOW */
    DL_GPIO_initDigitalOutput(MOTOR_PH2_IOMUX);
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0);

    /* CCR=period → 0% duty; init-low edge-aligned: output stays LOW */
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, MOTOR_PWM_PERIOD,
                                     MOTOR_PWM_IDX);
}

void motor_run(int16_t speed)
{
    uint16_t duty;
    uint16_t ccr;

    if (speed > MOTOR_SPEED_MAX)  speed = MOTOR_SPEED_MAX;
    if (speed < -MOTOR_SPEED_MAX) speed = -MOTOR_SPEED_MAX;

    if (speed == 0) {
        motor_coast();
        return;
    }

    duty = (uint16_t)(speed > 0 ? speed : -speed);
    ccr  = MOTOR_PWM_PERIOD - duty;

    if (speed > 0) {
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0);   /* PH2 = HIGH → fwd */
    } else {
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0); /* PH2 = LOW  → rev */
    }

    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, ccr, MOTOR_PWM_IDX);
}

void motor_coast(void)
{
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, MOTOR_PWM_PERIOD,
                                     MOTOR_PWM_IDX);
}

void motor_brake(void)
{
    /* DRV8874 slow-decay brake: PWM=100% → CCR=0 */
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, 0, MOTOR_PWM_IDX);
}
