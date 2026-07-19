#include "imu_task.h"
#include "bmi088driver.h"
#include "MahonyAHRS.h"
#include "tim.h"
#include "qd4310.h"
#include "can_bsp.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>

#define DES_TEMP    40.0f
#define KP          100.f
#define KI          50.f
#define KD          10.f
#define MAX_OUT     500

/* ── 角度闭环参数 ── */
#define ANGLE_SETTLE_DEG        2.0f
#define YAW_SPEED_KP            6.0f
#define YAW_SPEED_MAX          40.0f
#define YAW_SPEED_MIN          20.0f
#define YAW_SPEED_MIN_ERR       5.0f
#define YAW_ACCEL_RPM_S       400.0f
#define YAW_MOTOR_DIR           1.0f

/* PC13 电机固定角度 = 4.51 rad */
#define PC13_FIXED_ANGLE      4.51f

float gyro[3] = {0.0f};
float acc[3] = {0.0f};
static float temp = 0.0f;

float imuQuat[4] = {0.0f};
float imuAngle[3] = {0.0f};

float out = 0;
float err = 0, err_l = 0, err_ll = 0;

volatile uint32_t can_tx_cnt = 0;
volatile uint32_t can_err_cnt = 0;

uint8_t  motor_enabled = 1;
uint8_t  calib_mode   = 1;    /* 标定模式：1=标定, 0=正常运行 */
float g_shaft_angle = 0.0f;

static float angle_wrap_180(float e) {
    while(e>180) e-=360; while(e<-180) e+=360; return e;
}

static float get_shaft_angle(float q[4]) {
    float gx=2*(q[1]*q[3]-q[0]*q[2]);
    float gy=2*(q[0]*q[1]+q[2]*q[3]);
    return atan2f(gx,gy)*57.295779513f;
}

static float limitf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void AHRS_init(float q[4])   { q[0]=1; q[1]=0; q[2]=0; q[3]=0; }
void AHRS_update(float q[4], float g[3], float a[3])
    { MahonyAHRSupdateIMU(q,g[0],g[1],g[2], a[0],a[1],a[2]); }
void GetAngle(float q[4], float *y, float *p, float *r) {
    *y=atan2f(2*(q[0]*q[3]+q[1]*q[2]), 2*(q[0]*q[0]+q[1]*q[1])-1);
    *p=asinf(-2*(q[1]*q[3]-q[0]*q[2]));
    *r=atan2f(2*(q[0]*q[1]+q[2]*q[3]), 2*(q[0]*q[0]+q[3]*q[3])-1);
}

void ImuTask_Entry(void const * argument)
{
    osDelay(500);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    CDC_Transmit_HS((uint8_t*)"BOOT\r\n", 6);

    while(BMI088_init()) { osDelay(100); }

    BMI088_read(gyro, acc, &temp);
    AHRS_init(imuQuat);

    CDC_Transmit_HS((uint8_t*)"IMU_OK\r\n", 8);

    QD4310_t motor1 = {.id = 0x00, .hfdcan = &hfdcan1};
    QD4310_t motor2 = {.id = 0x00, .hfdcan = &hfdcan2};
    osDelay(500);

    uint8_t m1e = QD4310_Enable(&motor1); osDelay(50);
    uint8_t m2e = QD4310_Enable(&motor2); osDelay(50);

    float target_angle = 42.00f;

    float shaft_angle;
    float speed_cmd = 0.0f;
    uint32_t control_tick = HAL_GetTick();
    uint32_t loop_count = 0;

    for(;;)
    {
        BMI088_read(gyro, acc, &temp);
        AHRS_update(imuQuat, gyro, acc);
        GetAngle(imuQuat,
            imuAngle+INS_YAW_ADDRESS_OFFSET,
            imuAngle+INS_PITCH_ADDRESS_OFFSET,
            imuAngle+INS_ROLL_ADDRESS_OFFSET);
        shaft_angle = get_shaft_angle(imuQuat);
        g_shaft_angle = shaft_angle;

        /* ── CDC 输出：IMU真实角度 + PC13固定值 ── */
        if (++loop_count >= 50) {
            loop_count = 0;
            char cdc_buf[64];
            int len = snprintf(cdc_buf, sizeof(cdc_buf),
                "%.2f,%.2f\r\n", shaft_angle, PC13_FIXED_ANGLE);
            CDC_Transmit_HS((uint8_t*)cdc_buf, len);
        }

        /* 温度PWM */
        err_ll=err_l; err_l=err; err=DES_TEMP-temp;
        out=KP*err+KI*(err+err_l+err_ll)+KD*(err-err_l);
        if(out>MAX_OUT)out=MAX_OUT; if(out<0)out=0;
        htim3.Instance->CCR4=(uint16_t)out;

        /* ── IMU角度闭环 -> QD4310速度命令 ── */
        if (motor_enabled && !calib_mode) {
            uint32_t now = HAL_GetTick();
            uint32_t dt_ms = now - control_tick;

            float angle_error = angle_wrap_180(target_angle - shaft_angle);
            float desired_speed = 0.0f;

            if (dt_ms > 0) {
                float max_delta = YAW_ACCEL_RPM_S * ((float)dt_ms * 0.001f);
                float delta_speed;

                control_tick = now;

                if (fabsf(angle_error) > ANGLE_SETTLE_DEG) {
                    desired_speed = YAW_MOTOR_DIR * angle_error * YAW_SPEED_KP;
                    desired_speed = limitf(desired_speed, -YAW_SPEED_MAX, YAW_SPEED_MAX);

                    if (fabsf(angle_error) > YAW_SPEED_MIN_ERR &&
                        fabsf(desired_speed) < YAW_SPEED_MIN) {
                        desired_speed = (desired_speed >= 0.0f) ? YAW_SPEED_MIN : -YAW_SPEED_MIN;
                    }
                }

                delta_speed = desired_speed - speed_cmd;
                if (delta_speed > max_delta) {
                    delta_speed = max_delta;
                } else if (delta_speed < -max_delta) {
                    delta_speed = -max_delta;
                }

                speed_cmd += delta_speed;

                if (desired_speed == 0.0f && fabsf(speed_cmd) < 1.0f) {
                    speed_cmd = 0.0f;
                }
            }

            /* motor1 (PC14) = IMU角度闭环，速度模式 */
            if (QD4310_SetSpeed(&motor1,  speed_cmd) == 0) can_tx_cnt++; else can_err_cnt++;
            /* motor2 (PC13) = 固定角度 4.51 rad，角度模式 */
            if (QD4310_SetAngle(&motor2,  PC13_FIXED_ANGLE) == 0) can_tx_cnt++; else can_err_cnt++;
        } else {
            if (speed_cmd != 0.0f) {
                speed_cmd = 0.0f;
                if (QD4310_SetSpeed(&motor1, 0.0f) == 0) can_tx_cnt++; else can_err_cnt++;
                if (QD4310_SetSpeed(&motor2, 0.0f) == 0) can_tx_cnt++; else can_err_cnt++;
            }
        }

        osDelay(1);
    }
}
