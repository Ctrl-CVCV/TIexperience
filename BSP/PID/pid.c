#include "pid.h"
#include <math.h>

// PID初始化
void PID_Init(PID_Controller* pid, float kp, float ki, float kd, float output_limit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_limit = output_limit;
    pid->integral_limit = output_limit * 2.0f; // 积分限幅为输出的两倍
    pid->dead_zone = 0.0f;

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->last_time = 0;
}

// PID参数设置
void PID_SetParams(PID_Controller* pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

// PID重置
void PID_Reset(PID_Controller* pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->last_time = 0;
}

// 位置式PID更新，返回绝对u，调用方用 u = PID_Update(...)
float PID_Update(PID_Controller* pid, float setpoint, float measurement, float dt)
{
    float error = setpoint - measurement;
    float derivative;
    float output;

    /* 死区 */
    if (fabsf(error) < pid->dead_zone) {
        error = 0.0f;
    }

    /* 积分累加并限幅 */
    pid->integral += error * dt;
    if (pid->integral > pid->integral_limit)
        pid->integral = pid->integral_limit;
    else if (pid->integral < -pid->integral_limit)
        pid->integral = -pid->integral_limit;

    /* 微分：基于measurement变化（-dM/dt），避免setpoint突变冲击 */
    derivative = 0.0f;
    if (dt > 0.0001f) {
        derivative = (pid->prev_error - error) / dt;
    }

    pid->prev_error = error;

    /* 位置式：u = Kp*e + Ki*∫e·dt + Kd*de/dt */
    output = pid->kp * error
           + pid->ki * pid->integral
           + pid->kd * derivative;

    /* 输出限幅 */
    if (output > pid->output_limit)
        output = pid->output_limit;
    else if (output < -pid->output_limit)
        output = -pid->output_limit;

    return output;
}
