/*
 * motor.c - DRV8874 dual motor control (PH/EN mode)
 *
 * Motor A: EN1=PA29 (TIMG6 CCP0), PH1=PA31, nSLEEP1=PB4, nFAULT1=PB6
 * Motor B: EN2=PA30 (TIMG6 CCP1), PH2=PB0,  nSLEEP2=PB5, nFAULT2=PB7
 *
 * DRV8874 PH/EN truth table:
 *   nSLEEP=H, PH=H, EN=PWM  → Forward (Out1=H, Out2=L in PWM ON)
 *   nSLEEP=H, PH=L, EN=PWM  → Reverse (Out1=L, Out2=H in PWM ON)
 *   nSLEEP=L                → Coast / Sleep (H-bridge Hi-Z)
 *   nSLEEP=H, EN=L (0% PWM) → Brake (slow decay, both low-side ON)
 *
 * Direction swap: set to 1 if motor spins opposite to expected direction
 */
#include "motor.h"
#include "../../ti_msp_dl_config.h"

/* ---- Direction swap (change if motor spins the wrong way) ---- */
#define MOTOR_A_DIR_SWAP  0
#define MOTOR_B_DIR_SWAP  1

/* ---- Motor A pins ---- */
#define MOTOR_A_EN_PORT       GPIOA
#define MOTOR_A_EN_PIN        DL_GPIO_PIN_29
#define MOTOR_A_EN_IOMUX      (IOMUX_PINCM4)
#define MOTOR_A_EN_IOMUX_FUNC IOMUX_PINCM4_PF_TIMG6_CCP0

#define MOTOR_A_PH_PORT       GPIOA
#define MOTOR_A_PH_PIN        DL_GPIO_PIN_31
#define MOTOR_A_PH_IOMUX      (IOMUX_PINCM6)

#define MOTOR_A_NSLEEP_PORT   GPIOB
#define MOTOR_A_NSLEEP_PIN    DL_GPIO_PIN_4
#define MOTOR_A_NSLEEP_IOMUX  (IOMUX_PINCM17)

#define MOTOR_A_NFAULT_PORT   GPIOB
#define MOTOR_A_NFAULT_PIN    DL_GPIO_PIN_6
#define MOTOR_A_NFAULT_IOMUX  (IOMUX_PINCM23)

/* ---- Motor B pins ---- */
#define MOTOR_B_EN_PORT       GPIOA
#define MOTOR_B_EN_PIN        DL_GPIO_PIN_30
#define MOTOR_B_EN_IOMUX      (IOMUX_PINCM5)
#define MOTOR_B_EN_IOMUX_FUNC IOMUX_PINCM5_PF_TIMG6_CCP1

#define MOTOR_B_PH_PORT       GPIOB
#define MOTOR_B_PH_PIN        DL_GPIO_PIN_0
#define MOTOR_B_PH_IOMUX      (IOMUX_PINCM12)

#define MOTOR_B_NSLEEP_PORT   GPIOB
#define MOTOR_B_NSLEEP_PIN    DL_GPIO_PIN_5
#define MOTOR_B_NSLEEP_IOMUX  (IOMUX_PINCM18)

#define MOTOR_B_NFAULT_PORT   GPIOB
#define MOTOR_B_NFAULT_PIN    DL_GPIO_PIN_7
#define MOTOR_B_NFAULT_IOMUX  (IOMUX_PINCM24)

/* ---- TIMG6 PWM ---- */
#define MOTOR_PWM_INST        TIMG6
#define MOTOR_PWM_CC_A_IDX    DL_TIMER_CC_0_INDEX
#define MOTOR_PWM_CC_B_IDX    DL_TIMER_CC_1_INDEX

void motor_init(void)
{
    /* ---- TIMG6 power & reset (not in auto-generated init yet) ---- */
    DL_TimerG_reset(MOTOR_PWM_INST);
    DL_TimerG_enablePower(MOTOR_PWM_INST);
    delay_cycles(POWER_STARTUP_DELAY);

    /* ---- TIMG6 PWM init (stop, config, then start) ---- */
    DL_TimerG_setClockConfig(MOTOR_PWM_INST,
        &(DL_TimerG_ClockConfig){
            .clockSel    = DL_TIMER_CLOCK_BUSCLK,
            .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
            .prescale    = 0U
        });

    DL_TimerG_initPWMMode(MOTOR_PWM_INST,
        &(DL_TimerG_PWMConfig){
            .pwmMode            = DL_TIMER_PWM_MODE_EDGE_ALIGN,
            .period             = MOTOR_PWM_PERIOD,
            .isTimerWithFourCC  = false,
            .startTimer         = DL_TIMER_STOP   /* start after full config */
        });

    /* ---- PWM output pins (EN1=PA29, EN2=PA30) — must config before PWM starts ---- */
    DL_GPIO_initPeripheralOutputFunction(MOTOR_A_EN_IOMUX, MOTOR_A_EN_IOMUX_FUNC);
    DL_GPIO_enableOutput(MOTOR_A_EN_PORT, MOTOR_A_EN_PIN);
    DL_GPIO_initPeripheralOutputFunction(MOTOR_B_EN_IOMUX, MOTOR_B_EN_IOMUX_FUNC);
    DL_GPIO_enableOutput(MOTOR_B_EN_PORT, MOTOR_B_EN_PIN);

    /* CCP output: init low, go high when counter >= CCR.
     * CCR = period → output stays LOW initially (counter never reaches period) */
    DL_TimerG_setCaptureCompareOutCtl(MOTOR_PWM_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        MOTOR_PWM_CC_A_IDX);
    DL_TimerG_setCaptCompUpdateMethod(MOTOR_PWM_INST,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, MOTOR_PWM_CC_A_IDX);
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST,
        MOTOR_PWM_PERIOD, MOTOR_PWM_CC_A_IDX);

    DL_TimerG_setCaptureCompareOutCtl(MOTOR_PWM_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        MOTOR_PWM_CC_B_IDX);
    DL_TimerG_setCaptCompUpdateMethod(MOTOR_PWM_INST,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, MOTOR_PWM_CC_B_IDX);
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST,
        MOTOR_PWM_PERIOD, MOTOR_PWM_CC_B_IDX);

    /* Counter zero/adv/load control by CCCTL0 */
    DL_TimerG_setCounterControl(MOTOR_PWM_INST,
        DL_TIMER_CZC_CCCTL0_ZCOND, DL_TIMER_CAC_CCCTL0_ACOND,
        DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerG_setCCPDirection(MOTOR_PWM_INST,
        DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT);
    DL_TimerG_enableShadowFeatures(MOTOR_PWM_INST);

    DL_TimerG_enableClock(MOTOR_PWM_INST);
    DL_TimerG_startCounter(MOTOR_PWM_INST);

    /* ---- GPIO output pins (PH1=PA31, PH2=PB0, nSLEEP1=PB4, nSLEEP2=PB5) ---- */
    DL_GPIO_initDigitalOutput(MOTOR_A_PH_IOMUX);
    DL_GPIO_initDigitalOutput(MOTOR_B_PH_IOMUX);
    DL_GPIO_initDigitalOutput(MOTOR_A_NSLEEP_IOMUX);
    DL_GPIO_initDigitalOutput(MOTOR_B_NSLEEP_IOMUX);

    DL_GPIO_enableOutput(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);
    DL_GPIO_enableOutput(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
    DL_GPIO_enableOutput(MOTOR_A_NSLEEP_PORT, MOTOR_A_NSLEEP_PIN);
    DL_GPIO_enableOutput(MOTOR_B_NSLEEP_PORT, MOTOR_B_NSLEEP_PIN);

    /* ---- GPIO input pins with pull-up (nFAULT1=PB6, nFAULT2=PB7) ---- */
    DL_GPIO_initDigitalInputFeatures(MOTOR_A_NFAULT_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(MOTOR_B_NFAULT_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* Start with motors in coast (nSLEEP low, PH low, PWM off) */
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

    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, ccr, MOTOR_PWM_CC_A_IDX);

    /* Wake up driver */
    DL_GPIO_setPins(MOTOR_A_NSLEEP_PORT, MOTOR_A_NSLEEP_PIN);

#if MOTOR_A_DIR_SWAP
    if (speed > 0) {
        DL_GPIO_clearPins(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);  /* Forward → PH=L */
    } else {
        DL_GPIO_setPins(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);    /* Reverse → PH=H */
    }
#else
    if (speed > 0) {
        DL_GPIO_setPins(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);    /* Forward → PH=H */
    } else {
        DL_GPIO_clearPins(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);  /* Reverse → PH=L */
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

    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, ccr, MOTOR_PWM_CC_B_IDX);

    /* Wake up driver */
    DL_GPIO_setPins(MOTOR_B_NSLEEP_PORT, MOTOR_B_NSLEEP_PIN);

#if MOTOR_B_DIR_SWAP
    if (speed > 0) {
        DL_GPIO_clearPins(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
    } else {
        DL_GPIO_setPins(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
    }
#else
    if (speed > 0) {
        DL_GPIO_setPins(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
    }
#endif
}

/* Coast: nSLEEP = L → H-bridge Hi-Z */
void motor_a_coast(void)
{
    DL_GPIO_clearPins(MOTOR_A_NSLEEP_PORT, MOTOR_A_NSLEEP_PIN);
    DL_GPIO_clearPins(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);
}

void motor_b_coast(void)
{
    DL_GPIO_clearPins(MOTOR_B_NSLEEP_PORT, MOTOR_B_NSLEEP_PIN);
    DL_GPIO_clearPins(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
}

/* Brake: nSLEEP = H, EN = L → slow decay (both low-side FETs ON) */
void motor_a_brake(void)
{
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST,
        MOTOR_PWM_PERIOD, MOTOR_PWM_CC_A_IDX);
    DL_GPIO_setPins(MOTOR_A_NSLEEP_PORT, MOTOR_A_NSLEEP_PIN);
    DL_GPIO_clearPins(MOTOR_A_PH_PORT, MOTOR_A_PH_PIN);
}

void motor_b_brake(void)
{
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST,
        MOTOR_PWM_PERIOD, MOTOR_PWM_CC_B_IDX);
    DL_GPIO_setPins(MOTOR_B_NSLEEP_PORT, MOTOR_B_NSLEEP_PIN);
    DL_GPIO_clearPins(MOTOR_B_PH_PORT, MOTOR_B_PH_PIN);
}
