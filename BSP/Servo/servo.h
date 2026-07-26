#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/*
 * Servo PWM driver — TIMA0 CCP2 on PA8.
 *
 * PWM: 50Hz (20ms period), 0.5ms–2.5ms pulse
 *   0.5ms → 0 degrees
 *   1.5ms → 90 degrees (center)
 *   2.5ms → 180 degrees
 */

void servo_init(void);
void servo_set_angle(uint8_t angle_deg);  /* 0 ~ 180 */

#endif
