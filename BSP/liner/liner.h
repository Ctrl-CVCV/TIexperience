#ifndef LINER_H
#define LINER_H

#include <stdint.h>
#include <stdbool.h>
#include "pid.h"

/* ---- Configuration ---- */
#define WHEEL_DISTANCE  0.10f   /* wheel track width (m) */

/* ---- Fork types ---- */
#define FORK_NONE       0U
#define FORK_LEFT       1U
#define FORK_RIGHT      2U
#define FORK_CROSS      3U
#define FORK120_NONE    0U
#define FORK120_BI      1U

/* ---- Global state ---- */
extern int   liner_states[8];
extern int   choose_states[3];
extern float ave_values_after_filtered;
extern float last_ave_value;
extern float ave_value_history[5];
extern uint8_t is_lost_line;
extern float weight[8];

/* Target RPM for each motor (line-following output) */
extern float target_speed_rpm1;
extern float target_speed_rpm2;
extern float liner_ref_rpm;   /* base speed RPM */

/* ---- API ---- */
void liner_init(void);
void liner_start(void);
void liner_pause(void);
void liner_resume(void);
void liner_set_pid(PID_Controller* pid);
void liner_choose(int choose_state[]);
void Read_line(void);

float    ave_value(void);
void     value_after_filtered(void);
void     liner_control2(float ref_rpm, PID_Controller* liner_controler);
void     turning(uint8_t mode, float ref_rpm, float turning_radius);
void     turning_2(uint8_t mode, float ref_rpm);
void     turning2_with_PID(uint8_t mode, float ref_rpm, PID_Controller* turning_pid,
                           float ang[], float target_ang);

uint8_t  turning_detect(void);
uint8_t  fork_detect(void);
uint8_t  fork120_detect(void);

float    angle_diff_deg(float a, float b);
float    angle_turning(uint8_t mode, float angle);

#endif
