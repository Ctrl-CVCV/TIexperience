/*
 * liner.c - 8-ch grayscale line-following sensor (CD4051 mux)
 *
 * Pin mapping (STM32 → MSPM0):
 *   PD2  → PC4  (mux data input)
 *   PC3  → PC7  (selector bit 2, MSB)
 *   PC11 → PC2  (selector bit 1)
 *   PC5  → PC3  (selector bit 0, LSB)
 *
 * TIMG14 ISR: reads sensors @ 200Hz
 * All control functions output target PWM directly (range 0-1000).
 */

#include "liner.h"
#include "../bsp.h"
#include "ti_msp_dl_config.h"

#define LINER_TIMER      TIMER_0_INST
#define LINER_TIMER_IRQN (TIMER_0_INST_INT_IRQN)

/* Selector bits (SysConfig-defined pins) */
#define SEL0_PORT  Liner_OUT0_PORT
#define SEL0_PIN   Liner_OUT0_PIN
#define SEL0_IOMUX Liner_OUT0_IOMUX

#define SEL1_PORT  Liner_OUT1_PORT
#define SEL1_PIN   Liner_OUT1_PIN
#define SEL1_IOMUX Liner_OUT1_IOMUX

#define SEL2_PORT  Liner_OUT2_PORT
#define SEL2_PIN   Liner_OUT2_PIN
#define SEL2_IOMUX Liner_OUT2_IOMUX

/* Mux data input (SysConfig-defined pin) */
#define DATA_PORT  Liner_IN_PORT
#define DATA_PIN   Liner_IN_PIN
#define DATA_IOMUX Liner_IN_IOMUX

int  choose_states[3];
int  liner_states[8];
float ave_values_after_filtered;
float last_ave_value;
float ave_value_history[5];
uint8_t is_lost_line;

float weight[8] = {-0.35f, -0.20f, -0.12f, -0.06f, 0.06f, 0.12f, 0.20f, 0.35f};

float target_speed_rpm1;
float target_speed_rpm2;

static bool liner_paused;

float liner_ref_rpm = 1000.0f;       /* base speed RPM */
static PID_Controller* liner_pid;   /* liner PD controller */

void liner_set_pid(PID_Controller* pid) { liner_pid = pid; }

void liner_init(void)
{
    /* Pins already configured by SYSCFG_DL_init().
     * Just ensure mux selectors start LOW.
     */
    DL_GPIO_clearPins(SEL0_PORT, SEL0_PIN);
    DL_GPIO_clearPins(SEL1_PORT, SEL1_PIN);
    DL_GPIO_clearPins(SEL2_PORT, SEL2_PIN);
}

/* ---- Liner pause --- */
void liner_pause(void) { liner_paused = true; }
void liner_resume(void) { liner_paused = false; }

void liner_choose(int choose_state[])
{
    if (choose_state[0])
        DL_GPIO_setPins(SEL0_PORT, SEL0_PIN);
    else
        DL_GPIO_clearPins(SEL0_PORT, SEL0_PIN);

    if (choose_state[1])
        DL_GPIO_setPins(SEL1_PORT, SEL1_PIN);
    else
        DL_GPIO_clearPins(SEL1_PORT, SEL1_PIN);

    if (choose_state[2])
        DL_GPIO_setPins(SEL2_PORT, SEL2_PIN);
    else
        DL_GPIO_clearPins(SEL2_PORT, SEL2_PIN);
}

void Read_line(void)
{
    /* Sensor 1: sel = 000 */
    choose_states[0] = 0; choose_states[1] = 0; choose_states[2] = 0;
    liner_choose(choose_states);
    delay_us(3);
    liner_states[0] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 2: sel = 001 */
    choose_states[0] = 1; choose_states[1] = 0; choose_states[2] = 0;
    liner_choose(choose_states);
    delay_us(3);
    liner_states[1] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 3: sel = 010 */
    choose_states[0] = 0; choose_states[1] = 1; choose_states[2] = 0;
    liner_choose(choose_states);
    delay_us(5);
    liner_states[2] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 4: sel = 011 */
    choose_states[0] = 1; choose_states[1] = 1; choose_states[2] = 0;
    liner_choose(choose_states);
    delay_us(5);
    liner_states[3] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 5: sel = 100 */
    choose_states[0] = 0; choose_states[1] = 0; choose_states[2] = 1;
    liner_choose(choose_states);
    delay_us(3);
    liner_states[4] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 6: sel = 101 */
    choose_states[0] = 1; choose_states[1] = 0; choose_states[2] = 1;
    liner_choose(choose_states);
    delay_us(5);
    liner_states[5] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 7: sel = 110 */
    choose_states[0] = 0; choose_states[1] = 1; choose_states[2] = 1;
    liner_choose(choose_states);
    delay_us(5);
    liner_states[6] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    /* Sensor 8: sel = 111 */
    choose_states[0] = 1; choose_states[1] = 1; choose_states[2] = 1;
    liner_choose(choose_states);
    delay_us(3);
    liner_states[7] = (DL_GPIO_readPins(DATA_PORT, DATA_PIN) == DATA_PIN) ? 1 : 0;

    delay_us(3);
}

/* ---- Weighted average of line position ---- */
float ave_value(void)
{
    float sum = 0.0f;
    int   cnt = 0;

    for (int i = 0; i < 8; i++) {
        if (liner_states[i]) {
            sum += weight[i];
            cnt++;
        }
    }
    if (cnt == 0)
        return -3.0f; /* lost line */
    return sum / (float)cnt;
}

/* ---- EMA-filtered line position ---- */
void value_after_filtered(void)
{
    /* shift history */
    ave_value_history[4] = ave_value_history[3];
    ave_value_history[3] = ave_value_history[2];
    ave_value_history[2] = ave_value_history[1];
    ave_value_history[1] = ave_value_history[0];

    float current_value = ave_value();

    if (current_value < -2.5f) {
        is_lost_line = 1;
        if (last_ave_value != 0.0f) {
            ave_value_history[0] = last_ave_value;
        } else {
            ave_value_history[0] = (ave_value_history[1] + ave_value_history[2] +
                                    ave_value_history[3] + ave_value_history[4]) / 4.0f;
        }
    } else {
        is_lost_line = 0;
        ave_value_history[0] = current_value;
        last_ave_value = current_value;
    }

    ave_values_after_filtered = 0.7f * ave_value_history[0]
                              + 0.3f * ave_value_history[1];
}

/* ---- Line-following control: PID-based, outputs RPM setpoints ---- */
void liner_control2(float ref_rpm, PID_Controller* liner_controler)
{
    float output = PID_Update(liner_controler, 0.0f,
                              ave_values_after_filtered, 0.01f);
    target_speed_rpm1 = ref_rpm * (1.0f - output);
    target_speed_rpm2 = ref_rpm * (1.0f + output);
}

/* ---- Turning with radius (RPM setpoints) ---- */
void turning(uint8_t mode, float ref_rpm, float turning_radius)
{
    float kinner = (turning_radius - (WHEEL_DISTANCE / 2.0f)) / turning_radius;
    float kouter = (turning_radius + (WHEEL_DISTANCE / 2.0f)) / turning_radius;

    switch (mode) {
    case 0: /* straight */
        target_speed_rpm1 = ref_rpm;
        target_speed_rpm2 = ref_rpm;
        break;
    case 1: /* left turn */
        target_speed_rpm1 = ref_rpm * kinner;
        target_speed_rpm2 = ref_rpm * kouter;
        break;
    case 2: /* right turn */
        target_speed_rpm1 = ref_rpm * kouter;
        target_speed_rpm2 = ref_rpm * kinner;
        break;
    default: break;
    }
}

/* ---- Pivot turn in place (RPM setpoints) ---- */
void turning_2(uint8_t mode, float ref_rpm)
{
    if (mode == 1) { /* left */
        target_speed_rpm1 = -ref_rpm;
        target_speed_rpm2 =  ref_rpm;
    } else if (mode == 2) { /* right */
        target_speed_rpm1 =  ref_rpm;
        target_speed_rpm2 = -ref_rpm;
    }
}

/* ---- Pivot turn with IMU angle feedback (RPM setpoints) ---- */
void turning2_with_PID(uint8_t mode, float ref_rpm,
                       PID_Controller* turning_pid,
                       float ang[], float target_ang)
{
    float pidout = PID_Update(turning_pid,
                              angle_diff_deg(target_ang, ang[0]),
                              0.0f, 0.01f);
    if (mode == 1) { /* left */
        target_speed_rpm1 = -ref_rpm * pidout;
        target_speed_rpm2 =  ref_rpm * pidout;
    } else if (mode == 2) { /* right */
        target_speed_rpm1 =  ref_rpm * pidout;
        target_speed_rpm2 = -ref_rpm * pidout;
    }
}

/* ---- Corner detection ---- */
uint8_t turning_detect(void)
{
    if (liner_states[5] == 1 && liner_states[6] == 1 && liner_states[7] == 1)
        return 1; /* left */
    else if (liner_states[0] == 1 && liner_states[1] == 1 && liner_states[2] == 1)
        return 2; /* right */
    return 0; /* straight */
}

/* ---- Fork/cross detection with L/R/C discrimination ---- */
uint8_t fork_detect(void)
{
    uint8_t current_type = FORK_NONE;
    int gap_count = 0, max_gap_length = 0, current_gap = 0;

    for (int i = 0; i < 8; i++) {
        if (liner_states[i] == 0) {
            current_gap++;
        } else {
            if (current_gap > 0) {
                gap_count++;
                if (current_gap > max_gap_length) max_gap_length = current_gap;
                current_gap = 0;
            }
        }
    }
    if (current_gap > 0) {
        gap_count++;
        if (current_gap > max_gap_length) max_gap_length = current_gap;
    }

    if (gap_count >= 2 || max_gap_length >= 3) {
        int left_count  = liner_states[0] + liner_states[1] + liner_states[2];
        int right_count = liner_states[5] + liner_states[6] + liner_states[7];

        if (left_count > right_count)
            current_type = FORK_LEFT;
        else if (right_count > left_count)
            current_type = FORK_RIGHT;
        else
            current_type = FORK_CROSS;
    }

    static uint8_t last_type  = FORK_NONE;
    static uint8_t stable_cnt = 0U;

    if (current_type != FORK_NONE) {
        if (current_type == last_type) {
            if (stable_cnt < 3U) stable_cnt++;
        } else {
            last_type  = current_type;
            stable_cnt = 1U;
        }
        if (stable_cnt >= 2U) {
            uint8_t detected_type = current_type;
            last_type  = FORK_NONE;
            stable_cnt = 0U;
            return detected_type;
        }
    } else {
        last_type  = FORK_NONE;
        stable_cnt = 0U;
    }
    return FORK_NONE;
}

/* ---- 120° fork detection ---- */
uint8_t fork120_detect(void)
{
    int segments   = 0;
    int center_on  = liner_states[3] + liner_states[4];
    int left_on    = liner_states[0] + liner_states[1] + liner_states[2];
    int right_on   = liner_states[5] + liner_states[6] + liner_states[7];

    for (int i = 0; i < 8; i++) {
        if ((liner_states[i] != 0) && ((i == 0) || (liner_states[i - 1] == 0)))
            segments++;
    }

    if ((segments >= 2) && (center_on == 0) && (left_on >= 1) && (right_on >= 1))
        return FORK120_BI;

    return FORK120_NONE;
}

/* ---- Angle utilities ---- */
float angle_diff_deg(float a, float b)
{
    float diff = a - b;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

float angle_turning(uint8_t mode, float angle)
{
    float target = angle;
    if (mode == 1) { /* left */
        target = angle + 90.0f;
        if (target >  180.0f) target -= 360.0f;
    } else if (mode == 2) { /* right */
        target = angle - 90.0f;
        if (target < -180.0f) target += 360.0f;
    }
    return target;
}

/* ---- TIMG14 periodic read @ 200Hz ---- */
void liner_start(void)
{
    DL_TimerG_ClockConfig clk = {
        .clockSel    = DL_TIMER_CLOCK_BUSCLK,
        .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
        .prescale    = 0U,
    };
    DL_TimerG_TimerConfig tmr = {
        .period    = 49999,
        .timerMode = DL_TIMER_TIMER_MODE_PERIODIC,
        .startTimer = DL_TIMER_START,
    };

    DL_TimerG_setClockConfig(LINER_TIMER, &clk);
    DL_TimerG_initTimerMode(LINER_TIMER, &tmr);
    DL_TimerG_enableInterrupt(LINER_TIMER, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_SetPriority(LINER_TIMER_IRQN, 3);
    NVIC_EnableIRQ(LINER_TIMER_IRQN);
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(LINER_TIMER)) {
    case DL_TIMER_IIDX_ZERO:
        DL_TimerG_clearInterruptStatus(LINER_TIMER,
            DL_TIMERG_INTERRUPT_ZERO_EVENT);
        Read_line();
        value_after_filtered();
        if (!liner_paused && liner_pid) {
            liner_control2(liner_ref_rpm, liner_pid);
            /* target_speed_rpm1/2 now set; main loop PID tracks them */
        }
        break;
    default: break;
    }
}
