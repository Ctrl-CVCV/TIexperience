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

/* motor1 三个方向对应角度 (rad) */
#define MOTOR1_ANGLE_FRONT   0.25f
#define MOTOR1_ANGLE_LEFT    2.5f
#define MOTOR1_ANGLE_RIGHT   4.2f

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
uint8_t  calib_mode   = 0;    /* 标定模式：1=标定, 0=正常运行 */
float g_shaft_angle = 0.0f;

static float get_shaft_angle(float q[4]) {
    float gx=2*(q[1]*q[3]-q[0]*q[2]);
    float gy=2*(q[0]*q[1]+q[2]*q[3]);
    return atan2f(gx,gy)*57.295779513f;
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

    float shaft_angle;
    uint32_t loop_count = 0;
    float motor1_target_angle = MOTOR1_ANGLE_FRONT;

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

        /* ── motor1 角度控制：根据 CDC 方向命令切换目标角度 ── */
        if (motor_enabled && !calib_mode) {
            if (g_motor1_dir_cmd_valid) {
                g_motor1_dir_cmd_valid = 0;
                if (g_motor1_dir_cmd == MOTOR1_DIR_FRONT) {
                    motor1_target_angle = MOTOR1_ANGLE_FRONT;
                } else if (g_motor1_dir_cmd == MOTOR1_DIR_LEFT) {
                    motor1_target_angle = MOTOR1_ANGLE_LEFT;
                } else if (g_motor1_dir_cmd == MOTOR1_DIR_RIGHT) {
                    motor1_target_angle = MOTOR1_ANGLE_RIGHT;
                }
            }

            if (QD4310_SetAngle(&motor1, motor1_target_angle) == 0) can_tx_cnt++; else can_err_cnt++;
            /* motor2 (PC13) = 固定角度 4.51 rad，角度模式 */
            if (QD4310_SetAngle(&motor2,  PC13_FIXED_ANGLE) == 0) can_tx_cnt++; else can_err_cnt++;
        } else {
            if (QD4310_SetSpeed(&motor1, 0.0f) == 0) can_tx_cnt++; else can_err_cnt++;
            if (QD4310_SetSpeed(&motor2, 0.0f) == 0) can_tx_cnt++; else can_err_cnt++;
        }

        osDelay(1);
    }
}
