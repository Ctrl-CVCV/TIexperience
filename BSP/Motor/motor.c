/*
 * motor.c - DRV8874 single motor control
 *
 * EN/IN1  = TIMG6 CH1 (PA30)  — PWM speed
 * PH/IN2  = PA31              — direction (HIGH=fwd, LOW=rev)
 * nSLEEP  = PB4               — HIGH=enable, LOW=sleep
 * nFault  = PB4               — input (ignore for now)
 */

#include "motor.h"
#include "../ti_msp_dl_config.h"

#define MOTOR_PWM_INST    MOTOR1_PWM_INST          /* TIMG6 */
#define MOTOR_PWM_IDX     DL_TIMER_CC_1_INDEX       /* CH1 */
#define MOTOR_PWM_PERIOD  1000

void motor_init(void)
{
    /* nSLEEP = PB4. SysConfig configured it as nFault (input), override. */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM17);  /* PB4 */
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_4);

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
        DL_GPIO_setPins(GPIO_MOTORS_PH1_PORT, GPIO_MOTORS_PH1_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MOTORS_PH1_PORT, GPIO_MOTORS_PH1_PIN);
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
