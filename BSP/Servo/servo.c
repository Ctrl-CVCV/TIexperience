/*
 * servo.c — TIMA0 CCP2 50Hz servo PWM on PA8.
 *
 * Hardware:
 * - PA8 (PINCM8) → TIMA0 CCP2
 * - Timer clock: 80MHz / 64 = 1.25MHz
 * - Period: 25000 = 20ms (50Hz)
 * - Pulse: 625 (0.5ms) to 3125 (2.5ms)
 */

#include "servo.h"
#include "ti_msp_dl_config.h"

#define SERVO_INST           TIMA0
#define SERVO_CC_IDX         DL_TIMERA_CAPTURE_COMPARE_2_INDEX
#define SERVO_CC_OUTPUT      DL_TIMER_CC2_OUTPUT

/* 1.25MHz / 50Hz = 25000 */
#define SERVO_PERIOD         25000U
#define SERVO_MIN_PULSE      625     /* 0.5ms → 0 degrees   */
#define SERVO_MAX_PULSE      3125    /* 2.5ms → 180 degrees */
#define SERVO_CENTER_PULSE   1875    /* 1.5ms → 90 degrees  */

void servo_init(void)
{
    DL_TimerA_reset(SERVO_INST);
    DL_TimerA_enablePower(SERVO_INST);
    delay_cycles(POWER_STARTUP_DELAY);

    /* Clock: 80MHz / (1 * (63+1)) = 1.25MHz */
    DL_TimerA_setClockConfig(SERVO_INST,
        &(DL_TimerA_ClockConfig){
            .clockSel    = DL_TIMER_CLOCK_BUSCLK,
            .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
            .prescale    = 63U
        });

    /* PWM: edge-aligned, 50Hz */
    DL_TimerA_initPWMMode(SERVO_INST,
        &(DL_TimerA_PWMConfig){
            .pwmMode           = DL_TIMER_PWM_MODE_EDGE_ALIGN,
            .period            = SERVO_PERIOD,
            .isTimerWithFourCC = true,
            .startTimer        = DL_TIMER_STOP
        });

    /* PA8 pin mux → TIMA0 CCP2 */
    DL_GPIO_initPeripheralOutputFunction(
        IOMUX_PINCM8, IOMUX_PINCM8_PF_TIMA0_CCP2);
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_8);

    /* CCP2 output: init low, direct func value, immediate update */
    DL_TimerA_setCaptureCompareOutCtl(SERVO_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        SERVO_CC_IDX);

    DL_TimerA_setCaptCompUpdateMethod(SERVO_INST,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, SERVO_CC_IDX);

    DL_TimerA_setCaptureCompareValue(SERVO_INST,
        SERVO_CENTER_PULSE, SERVO_CC_IDX);

    /* Counter control — use CCCTL2 since CCP2 is the active channel */
    DL_TimerA_setCounterControl(SERVO_INST,
        DL_TIMER_CZC_CCCTL2_ZCOND,
        DL_TIMER_CAC_CCCTL2_ACOND,
        DL_TIMER_CLC_CCCTL2_LCOND);

    DL_TimerA_setCCPDirection(SERVO_INST, SERVO_CC_OUTPUT);

    DL_TimerA_enableClock(SERVO_INST);
    DL_TimerA_startCounter(SERVO_INST);
}

void servo_set_angle(uint8_t angle_deg)
{
    uint16_t pulse;

    if (angle_deg > 180U) {
        angle_deg = 180U;
    }

    pulse = SERVO_MIN_PULSE
          + (uint16_t)angle_deg * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180U;

    DL_TimerA_setCaptureCompareValue(SERVO_INST, pulse, SERVO_CC_IDX);
}
