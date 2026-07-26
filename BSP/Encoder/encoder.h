#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* 500-line encoder, x4 quad = 2000 counts/motor-rev, 1:20 gearing */
#define ENCODER_CPR       2000
#define ENCODER_REDUCTION 20

/* Speed sample rate = TIMER_12(1000Hz) / 2 = 500Hz */
#define ENCODER_HZ        500

void encoder_init(void);

/* Motor shaft RPM (before reduction). Negative = reverse. */
int16_t encoder_a_get_rpm(void);
int16_t encoder_b_get_rpm(void);

/* Low-pass filtered RPM (EMA, for smoother control / telemetry) */
int16_t encoder_a_get_rpm_filt(void);
int16_t encoder_b_get_rpm_filt(void);

/* Output shaft RPM (after reduction). Negative = reverse. */
static inline int16_t encoder_a_get_output_rpm(void) {
    return encoder_a_get_rpm() / ENCODER_REDUCTION;
}
static inline int16_t encoder_b_get_output_rpm(void) {
    return encoder_b_get_rpm() / ENCODER_REDUCTION;
}

/* Accumulated position (raw counts) */
int32_t encoder_a_get_pos(void);
int32_t encoder_b_get_pos(void);

/* Debug: ISR fire count (every 1ms tick) */
uint16_t encoder_get_isr_count(void);

/* Debug: raw QEI counter register values */
uint16_t encoder_get_raw_a(void);
uint16_t encoder_get_raw_b(void);

/* Debug: ISR internal values (delta, curr) */
int16_t  encoder_get_delta_a(void);
int16_t  encoder_get_delta_b(void);
uint16_t encoder_get_curr_a(void);
uint16_t encoder_get_curr_b(void);

/* Debug: unfiltered RPM (before EMA) */
int16_t encoder_get_raw_rpm_a(void);
int16_t encoder_get_raw_rpm_b(void);

#endif
