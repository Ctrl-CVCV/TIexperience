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
    pid->prev_prev_error = 0.0f;
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
    pid->prev_prev_error = 0.0f;
    pid->last_time = 0;
}

// 增量式PID更新，返回Δu，调用方用 u += Δu 累加
float PID_Update(PID_Controller* pid, float setpoint, float measurement, float dt)
{
    float error = setpoint - measurement;

    /* 死区 */
    if (fabsf(error) < pid->dead_zone) {
        error = 0.0f;
    }

    /*
     * 增量式（速度形式）：
     *   Δu = Kp*(e(k)-e(k-1)) + Ki*e(k)*dt + Kd*(e(k)-2*e(k-1)+e(k-2))/dt
     * 稳态时 error→0, Δe→0, Δ²e→0 → Δu=0, PWM保持不变
     */
    float p_delta = pid->kp * (error - pid->prev_error);
    float i_delta = pid->ki * error * dt;
    float d_delta = 0.0f;
    if (dt > 0.0001f) {
        d_delta = pid->kd * (error - 2.0f * pid->prev_error
                             + pid->prev_prev_error) / dt;
    }

    /* 保存历史 */
    pid->prev_prev_error = pid->prev_error;
    pid->prev_error = error;

    return p_delta + i_delta + d_delta;
}
