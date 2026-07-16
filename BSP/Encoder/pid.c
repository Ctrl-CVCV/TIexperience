#include "pid.h"
#include <math.h>

extern volatile uint32_t nowtime;  /* MSPM0 1ms counter */

void PID_Init(PID_Controller* pid, float kp, float ki, float kd, float output_limit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_limit = output_limit;
    pid->integral_limit = output_limit * 2.0f;
    pid->dead_zone = 0.0f;

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->last_time = nowtime;
}

void PID_SetParams(PID_Controller* pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void PID_Reset(PID_Controller* pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->last_time = nowtime;
}

float PID_Update(PID_Controller* pid, float setpoint, float measurement, float dt)
{
    pid->last_time = nowtime;

    float error = setpoint - measurement;

    /* Dead zone */
    if (fabsf(error) < pid->dead_zone) {
        error = 0.0f;
    }

    /* Proportional */
    float p_term = pid->kp * error;

    /* Integral with anti-windup */
    pid->integral += error * dt;
    if (pid->integral > pid->integral_limit) {
        pid->integral = pid->integral_limit;
    } else if (pid->integral < -pid->integral_limit) {
        pid->integral = -pid->integral_limit;
    }
    float i_term = pid->ki * pid->integral;

    /* Derivative-on-measurement (avoids setpoint kick) */
    float derivative = (measurement - pid->prev_measurement) / dt;
    float d_term = -pid->kd * derivative;

    pid->prev_error = error;
    pid->prev_measurement = measurement;

    float output = p_term + i_term + d_term;

    /* Output clamping */
    if (output > pid->output_limit) {
        output = pid->output_limit;
    } else if (output < -pid->output_limit) {
        output = -pid->output_limit;
    }

    return output;
}

