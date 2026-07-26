/*
 * encoder.c - Dual QEI encoder speed measurement
 *
 * QEI_0 = TIMG9 (PHA=PB29, PHB=PB30) → Motor A
 * QEI_1 = TIMG8 (PHA=PC6,  PHB=PA22) → Motor B
 *
 * TIMER_12 (TIMG12, 1000Hz) drives sampling at 500Hz (every 2 ticks).
 *
 * RPM = delta_counts * (60 * sample_hz) / CPR
 *     = delta * 60 * 500 / 2000
 *     = delta * 15
 */

#include "encoder.h"
#include "../../ti_msp_dl_config.h"

static volatile int32_t g_pos_a;
static volatile int32_t g_pos_b;
static volatile int16_t g_rpm_a;
static volatile int16_t g_rpm_b;
static volatile int16_t g_rpm_filt_a;
static volatile int16_t g_rpm_filt_b;

static uint16_t g_last_a;
static uint16_t g_last_b;
static uint8_t  g_tick;
static volatile uint16_t g_isr_count;

/* ISR debug: exposed for LCD display */
volatile int16_t  g_dbg_delta_a;
volatile int16_t  g_dbg_delta_b;
volatile uint16_t g_dbg_curr_a;
volatile uint16_t g_dbg_curr_b;
volatile uint16_t g_dbg_last_a;
volatile uint16_t g_dbg_last_b;

/* SysConfig sets TIMG12 as ONE_SHOT. Convert to periodic for encoder. */
void encoder_init(void)
{
    DL_TimerG_stopCounter(TIMER_12_INST);
    DL_TimerG_clearInterruptStatus(TIMER_12_INST,
        DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_TimerConfig cfg = {
        .period    = TIMER_12_INST_LOAD_VALUE,
        .timerMode = DL_TIMER_TIMER_MODE_PERIODIC,
        .startTimer = DL_TIMER_START,
    };
    DL_TimerG_initTimerMode(TIMER_12_INST, &cfg);
    DL_TimerG_enableInterrupt(TIMER_12_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_SetPriority(TIMER_12_INST_INT_IRQN, 1);
    NVIC_EnableIRQ(TIMER_12_INST_INT_IRQN);
    DL_TimerG_enableClock(TIMER_12_INST);
}

/* TIMG12 ISR: fires at 1000Hz, samples encoders every other tick (500Hz) */
void TIMER_12_INST_IRQHandler(void)
{
    uint16_t curr_a, curr_b;
    int16_t delta_a, delta_b;

    switch (DL_TimerG_getPendingInterrupt(TIMER_12_INST)) {
    case DL_TIMER_IIDX_ZERO:
        DL_TimerG_clearInterruptStatus(TIMER_12_INST,
            DL_TIMERG_INTERRUPT_ZERO_EVENT);

        g_isr_count++;
        g_tick ^= 1;
        if (g_tick) break;   /* skip odd ticks → 500Hz */

        curr_a = (uint16_t)QEI_0_INST->COUNTERREGS.CTR;
        curr_b = (uint16_t)QEI_1_INST->COUNTERREGS.CTR;

        delta_a = (int16_t)(curr_a - g_last_a);
        delta_b = (int16_t)(curr_b - g_last_b);

        /* ISR debug snapshot */
        g_dbg_curr_a = curr_a; g_dbg_curr_b = curr_b;
        g_dbg_last_a = g_last_a; g_dbg_last_b = g_last_b;
        g_dbg_delta_a = delta_a; g_dbg_delta_b = delta_b;

        g_last_a = curr_a;
        g_last_b = curr_b;

        g_pos_a += delta_a;
        g_pos_b += delta_b;

        /* RPM = delta * 60 * 500 / 2000 = delta * 15 */
        g_rpm_a = (int16_t)((int32_t)delta_a * 15);  /* positive = fwd */
        g_rpm_b = -(int16_t)((int32_t)delta_b * 15);

        /* EMA filter: filt = filt + (raw - filt) * 13 / 128 */
        {
            int16_t diff_a = g_rpm_a - g_rpm_filt_a;
            int16_t diff_b = g_rpm_b - g_rpm_filt_b;
            g_rpm_filt_a = (int16_t)((int32_t)g_rpm_filt_a + ((int32_t)diff_a * 13) / 128);
            g_rpm_filt_b = (int16_t)((int32_t)g_rpm_filt_b + ((int32_t)diff_b * 13) / 128);
        }
        break;

    default:
        break;
    }
}

int16_t encoder_a_get_rpm(void)        { return g_rpm_a; }
int16_t encoder_b_get_rpm(void)        { return g_rpm_b; }
int16_t encoder_a_get_rpm_filt(void)   { return g_rpm_filt_a; }
int16_t encoder_b_get_rpm_filt(void)   { return g_rpm_filt_b; }
int32_t encoder_a_get_pos(void)   { return g_pos_a; }
int32_t encoder_b_get_pos(void)   { return g_pos_b; }
uint16_t encoder_get_isr_count(void) { return g_isr_count; }
uint16_t encoder_get_raw_a(void)     { return (uint16_t)QEI_0_INST->COUNTERREGS.CTR; }
uint16_t encoder_get_raw_b(void)     { return (uint16_t)QEI_1_INST->COUNTERREGS.CTR; }
int16_t  encoder_get_delta_a(void)   { return g_dbg_delta_a; }
int16_t  encoder_get_delta_b(void)   { return g_dbg_delta_b; }
uint16_t encoder_get_curr_a(void)    { return g_dbg_curr_a; }
uint16_t encoder_get_curr_b(void)    { return g_dbg_curr_b; }
int16_t  encoder_get_raw_rpm_a(void) { return g_rpm_a; }
int16_t  encoder_get_raw_rpm_b(void) { return g_rpm_b; }
