#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/* Speed range: 0 = stop, MOTOR_PWM_PERIOD = full speed */
#define MOTOR_PWM_PERIOD  1000
#define MOTOR_SPEED_MAX   1000

void motor_init(void);
void motor_a_run(int16_t speed);
void motor_b_run(int16_t speed);
void motor_a_coast(void);
void motor_b_coast(void);
void motor_a_brake(void);
void motor_b_brake(void);

#endif
