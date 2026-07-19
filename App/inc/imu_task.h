#ifndef __IMU_TASK_H__
#define __IMU_TASK_H__

#include "cmsis_os.h"

#define INS_YAW_ADDRESS_OFFSET    0
#define INS_PITCH_ADDRESS_OFFSET  1
#define INS_ROLL_ADDRESS_OFFSET   2

extern float imuAngle[3];
extern float g_shaft_angle;       /* IMU 轴角度 */

#define MOTOR1_DIR_FRONT   0x00
#define MOTOR1_DIR_LEFT    0x01
#define MOTOR1_DIR_RIGHT   0x02

extern volatile uint8_t g_motor1_dir_cmd;
extern volatile uint8_t g_motor1_dir_cmd_valid;

#endif
