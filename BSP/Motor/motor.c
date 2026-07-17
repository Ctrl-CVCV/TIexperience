/*
 * motor.c - TB6612 dual motor control
 *
 * Motor A: AIN1=PB20, AIN2=PB24, PWMA=TIMA1 CH1 (PA31)
 * Motor B: BIN1=PA8,  BIN2=PA9,  PWMB=TIMA1 CH0 (PA28)
 *
 * Direction swap: set to 1 if motor spins opposite to expected direction
 */
#include "motor.h"
#include "../../ti_msp_dl_config.h"

/* ---- Direction swap (change if motor spins the wrong way) ---- */
#define MOTOR_A_DIR_SWAP  1
#define MOTOR_B_DIR_SWAP  1   /* encoder polarity fix */

/* ---- Motor A pin / PWM macros ---- */
#define MOTOR_A_IN1_PORT    ABIN_AIN1_PORT
#define MOTOR_A_IN1_PIN     ABIN_AIN1_PIN
#define MOTOR_A_IN2_PORT    ABIN_AIN2_PORT
#define MOTOR_A_IN2_PIN     ABIN_AIN2_PIN
#define MOTOR_A_PWM_INST    PWMA_INST
#define MOTOR_A_PWM_IDX     DL_TIMER_CC_1_INDEX

/* ---- Motor B pin / PWM macros ---- */
#define MOTOR_B_IN1_PORT    ABIN_BIN1_PORT
#define MOTOR_B_IN1_PIN     ABIN_BIN1_PIN
#define MOTOR_B_IN2_PORT    ABIN_BIN2_PORT
#define MOTOR_B_IN2_PIN     ABIN_BIN2_PIN
#define MOTOR_B_PWM_INST    PWMA_INST
#define MOTOR_B_PWM_IDX     DL_TIMER_CC_0_INDEX

void motor_init(void)
{
    /* Set PWM freq to 20kHz (above audible, below SysConfig 80kHz) */
    DL_TimerA_setLoadValue(PWMA_INST, MOTOR_PWM_PERIOD);
    motor_a_coast();
    motor_b_coast();
}

/* speed: -1000..1000, negative = reverse, 0 = coast */
void motor_a_run(int16_t speed)
{
    uint16_t duty;
    uint16_t ccr;

    if (speed > MOTOR_SPEED_MAX)  speed = MOTOR_SPEED_MAX;
    if (speed < -MOTOR_SPEED_MAX) speed = -MOTOR_SPEED_MAX;

    if (speed == 0) {
        motor_a_coast();
        return;
    }

    duty = (uint16_t)(speed > 0 ? speed : -speed);
    ccr  = MOTOR_PWM_PERIOD - (uint32_t)duty * MOTOR_PWM_PERIOD / MOTOR_SPEED_MAX;

    DL_TimerA_setCaptureCompareValue(MOTOR_A_PWM_INST, ccr, MOTOR_A_PWM_IDX);

#if MOTOR_A_DIR_SWAP
    if (speed > 0) {
        /* Forward → electrically reverse to compensate */
        DL_GPIO_clearPins(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
        DL_GPIO_setPins(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
    } else {
        /* Reverse → electrically forward */
        DL_GPIO_setPins(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
    }
#else
    if (speed > 0) {
        DL_GPIO_setPins(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
        DL_GPIO_setPins(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
    }
#endif
}

void motor_b_run(int16_t speed)
{
    uint16_t duty;
    uint16_t ccr;

    if (speed > MOTOR_SPEED_MAX)  speed = MOTOR_SPEED_MAX;
    if (speed < -MOTOR_SPEED_MAX) speed = -MOTOR_SPEED_MAX;

    if (speed == 0) {
        motor_b_coast();
        return;
    }

    duty = (uint16_t)(speed > 0 ? speed : -speed);
    ccr  = MOTOR_PWM_PERIOD - (uint32_t)duty * MOTOR_PWM_PERIOD / MOTOR_SPEED_MAX;

    DL_TimerA_setCaptureCompareValue(MOTOR_B_PWM_INST, ccr, MOTOR_B_PWM_IDX);

#if MOTOR_B_DIR_SWAP
    if (speed > 0) {
        DL_GPIO_clearPins(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN);
        DL_GPIO_setPins(MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN);
    } else {
        DL_GPIO_setPins(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN);
    }
#else
    if (speed > 0) {
        DL_GPIO_setPins(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN);
        DL_GPIO_setPins(MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN);
    }
#endif
}

void motor_a_coast(void)
{
    DL_GPIO_clearPins(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
}

void motor_b_coast(void)
{
    DL_GPIO_clearPins(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN);
}

void motor_a_brake(void)
{
    DL_GPIO_setPins(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
    DL_GPIO_setPins(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
}

void motor_b_brake(void)
{
    DL_GPIO_setPins(MOTOR_B_IN1_PORT, MOTOR_B_IN1_PIN);
    DL_GPIO_setPins(MOTOR_B_IN2_PORT, MOTOR_B_IN2_PIN);
}
