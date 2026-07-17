#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/* Speed range: 0 = stop, MOTOR_SPEED_MAX = full speed */
#define MOTOR_SPEED_MAX   1000

void motor_init(void);
void motor_run(int16_t speed);
void motor_coast(void);
void motor_brake(void);

#endif
