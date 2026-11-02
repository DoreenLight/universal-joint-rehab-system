/**
  ******************************************************************************
  * @file    motor_ctrl.c
  * @brief   Motor control operations — called by protocol dispatch
  ******************************************************************************
  */

#include "motor_ctrl.h"
#include "controller.h"
#include "tim.h"
#include "rs00_motor.h"
#include "flash_store.h"
#include "proto_app.h"
#include <stdio.h>

/* ============================ Motor1 (PID) ============================ */

void motor1_start(void)
{
    my_Order_mode.start_or_stop = _start;
    TIM2_Start_IT();
    My_state = stop_flag;
    printf("motor1: started\r\n");
}

void motor1_stop(void)
{
    my_Order_mode.start_or_stop = _stop;
    TIM2_Stop_IT();
    My_state = stop_flag;
    printf("motor1: stopped\r\n");
}

void motor1_set_angle(float angle)
{
    my_Order_mode.motor1_jiaodu = angle;
    printf("motor1: angle=%.4f\r\n", (double)angle);
}

void motor1_set_speed(float speed)
{
    my_Order_mode.motor1_sudu = speed;
    printf("motor1: speed=%.4f\r\n", (double)speed);
}

void motor1_set_current(float current)
{
    my_Order_mode.motor1_dianliu = current;
    printf("motor1: current=%.4f\r\n", (double)current);
}

void motor1_set_kp(float kp)
{
    my_Order_mode.motor1_kp = kp;
    Control_P_PID.kp = kp;
    (void)flash_store_set_param(PROTO_FLOAT_ID_MOTOR1_KP, kp);
    printf("motor1: Kp=%.4f (applied)\r\n", (double)kp);
}

void motor1_set_ki(float ki)
{
    my_Order_mode.motor1_ki = ki;
    Control_P_PID.ki = ki;
    (void)flash_store_set_param(PROTO_FLOAT_ID_MOTOR1_KI, ki);
    printf("motor1: Ki=%.4f (applied)\r\n", (double)ki);
}

/* --- Mode control --- */
void motor1_ctrl_set_mode(uint8_t mode)
{
    if (mode <= PWM_test)
    {
        my_Order_mode.mode = (ModeEnum)mode;
        printf("motor_ctrl: mode=%u ", (unsigned int)mode);
        switch (mode)
        {
        case 0u: printf("(position)\r\n"); break;
        case 1u: printf("(velocity)\r\n"); break;
        case 3u: printf("(PWM_test)\r\n"); break;
        default: break;
        }
    }
}

uint8_t motor1_ctrl_get_mode(void)
{
    return (uint8_t)my_Order_mode.mode;
}

/* --- PWM duty (used in PWM_test mode) --- */
static float g_pwm1_duty = 0.0f;

void motor1_set_pwm_duty(float duty)
{
    g_pwm1_duty = duty;
    printf("motor1: PWM duty=%.4f\r\n", (double)duty);
}

float motor1_get_pwm_duty(void) { return g_pwm1_duty; }

/* --- Max speed storage (deg/cycle) --- */
float g_m1_max_speed = 1.0f;  /* default 100 deg/s */
float g_m2_max_speed = 1.0f;

void motor1_set_max_speed(float deg_per_sec)
{
    g_m1_max_speed = deg_per_sec * 0.01f;
    if (g_m1_max_speed < 0.01f) g_m1_max_speed = 0.01f;
    (void)flash_store_set_param(PROTO_FLOAT_ID_MOTOR1_MAX_SPEED, g_m1_max_speed);
    printf("motor1: max_speed=%.1f deg/s\r\n", (double)deg_per_sec);
}
float motor1_get_max_speed(void) { return g_m1_max_speed; }

/* ============================ Motor2 (RS00 CAN) ============================ */

/* Cached control state — each setter updates one field + sends RS00_Control */
static float m2_pos    = 0.0f;
static float m2_vel    = 0.0f;
static float m2_torque = 0.0f;
static float m2_kp     = 0.0f;
static float m2_kd     = 0.0f;

void motor2_send_control(float pos, float vel, float torque, float kp, float kd)
{
    RS00_Control(0x7F, 0xFD, pos, vel, torque, kp, kd);
}

/* --- IO commands --- */

void motor2_enable(void)
{
    RS00_Enable(0x7F, 0xFD);
    printf("motor2: enabled\r\n");
}

void motor2_stop(void)
{
    RS00_Stop(0x7F, 0xFD);
    printf("motor2: stopped\r\n");
}

void motor2_set_mode(uint8_t mode)
{
    RS00_SetRunMode(0x7F, 0xFD, mode);
    printf("motor2: RS00 mode=%u\r\n", (unsigned int)mode);
}

void motor2_set_zero(void)
{
    RS00_SetZeroPosition(0x7F, 0xFD);
    printf("motor2: zero set\r\n");
}

void motor2_clear_error(void)
{
    RS00_StopAndClearError(0x7F, 0xFD);
    HAL_Delay(50);
    RS00_Enable(0x7F, 0xFD);
    printf("motor2: error cleared\r\n");
}

/* --- Float parameters --- */

/* --- Getters (for trajectory planner) --- */
float motor2_get_position(void)  { return m2_pos; }
float motor2_get_velocity(void)  { return m2_vel; }
float motor2_get_torque(void)    { return m2_torque; }
float motor2_get_kp(void)        { return m2_kp; }
float motor2_get_kd(void)        { return m2_kd; }
float motor2_get_max_speed(void) { return g_m2_max_speed; }

/* --- Setters: cache only, no CAN send. Trajectory planner sends in loop. --- */

void motor2_set_position(float pos)  { m2_pos = pos; }
void motor2_set_velocity(float vel)  { m2_vel = vel; }
void motor2_set_torque(float torque) { m2_torque = torque; }

void motor2_set_kp(float kp)
{
    m2_kp = kp;
    (void)flash_store_set_param(PROTO_FLOAT_ID_MOTOR2_KP, kp);
    printf("motor2: Kp=%.1f\r\n", (double)kp);
}

void motor2_set_kd(float kd)
{
    m2_kd = kd;
    (void)flash_store_set_param(PROTO_FLOAT_ID_MOTOR2_KD, kd);
    printf("motor2: Kd=%.1f\r\n", (double)kd);
}

void motor2_set_max_speed(float deg_per_sec)
{
    g_m2_max_speed = deg_per_sec * 0.01f;
    if (g_m2_max_speed < 0.01f) g_m2_max_speed = 0.01f;
    (void)flash_store_set_param(PROTO_FLOAT_ID_MOTOR2_MAX_SPEED, g_m2_max_speed);
    printf("motor2: max_speed=%.1f deg/s\r\n", (double)deg_per_sec);
}

void motor2_set_current_limit(float limit)
{
    RS00_WriteParamFloat(0x7F, 0xFD, 0x7018, limit);
    printf("motor2: current_limit=%.2f\r\n", (double)limit);
}

/* ============================ Rehab Passive ============================ */

static float m1_rehab_min    = 0.0f;
static float m1_rehab_max    = 30.0f;
static float m1_rehab_cycles = 5.0f;
static float m2_rehab_min    = 0.0f;
static float m2_rehab_max    = 30.0f;
static float m2_rehab_cycles = 5.0f;

/* --- Motor1 --- */
void motor1_set_rehab_min(float angle)  { m1_rehab_min = angle; }
void motor1_set_rehab_max(float angle)  { m1_rehab_max = angle; }
void motor1_set_rehab_cycles(float cyc) { m1_rehab_cycles = cyc; }
float    motor1_get_rehab_min(void)     { return m1_rehab_min; }
float    motor1_get_rehab_max(void)     { return m1_rehab_max; }
uint16_t motor1_get_rehab_cycles(void)  { return (uint16_t)(m1_rehab_cycles > 0.0f ? m1_rehab_cycles : 1.0f); }

/* --- Motor2 --- */
void motor2_set_rehab_min(float angle)  { m2_rehab_min = angle; }
void motor2_set_rehab_max(float angle)  { m2_rehab_max = angle; }
void motor2_set_rehab_cycles(float cyc) { m2_rehab_cycles = cyc; }
float    motor2_get_rehab_min(void)     { return m2_rehab_min; }
float    motor2_get_rehab_max(void)     { return m2_rehab_max; }
uint16_t motor2_get_rehab_cycles(void)  { return (uint16_t)(m2_rehab_cycles > 0.0f ? m2_rehab_cycles : 1.0f); }

/* ============================ Active Rehab (M1) ============================ */

static float m1_actv_speed    = 30.0f;   /* °/s */
static float m1_actv_limit1   = -30.0f;  /* ° */
static float m1_actv_limit2   = 30.0f;   /* ° */
static float m1_actv_pause    = 2.0f;    /* seconds */
static float m1_actv_tmax     = 5.0f;    /* N·m */
static float m1_actv_stiffness = 10.0f;   /* k */

void motor1_set_actv_speed(float spd)    { m1_actv_speed    = spd; }
void motor1_set_actv_limit1(float angle) { m1_actv_limit1   = angle; }
void motor1_set_actv_limit2(float angle) { m1_actv_limit2   = angle; }
void motor1_set_actv_pause(float sec)    { m1_actv_pause    = sec; }
void motor1_set_actv_tmax(float tmax)    { m1_actv_tmax     = tmax; }
void motor1_set_actv_stiffness(float k)  { m1_actv_stiffness = k; }
float motor1_get_actv_speed(void)        { return m1_actv_speed; }
float motor1_get_actv_limit1(void)       { return m1_actv_limit1; }
float motor1_get_actv_limit2(void)       { return m1_actv_limit2; }
float motor1_get_actv_pause(void)        { return m1_actv_pause; }
float motor1_get_actv_tmax(void)         { return m1_actv_tmax; }
float motor1_get_actv_stiffness(void)    { return m1_actv_stiffness; }

/* ============================ Active Rehab (M2) ============================ */

static float m2_actv_speed    = 30.0f;
static float m2_actv_limit1   = -30.0f;
static float m2_actv_limit2   = 30.0f;
static float m2_actv_pause    = 2.0f;
static float m2_actv_fmax     = 5.0f;
static float m2_actv_stiffness = 10.0f;

void motor2_set_actv_speed(float spd)    { m2_actv_speed    = spd; }
void motor2_set_actv_limit1(float angle) { m2_actv_limit1   = angle; }
void motor2_set_actv_limit2(float angle) { m2_actv_limit2   = angle; }
void motor2_set_actv_pause(float sec)    { m2_actv_pause    = sec; }
void motor2_set_actv_fmax(float fmax)    { m2_actv_fmax     = fmax; }
void motor2_set_actv_stiffness(float k)  { m2_actv_stiffness = k; }
float motor2_get_actv_speed(void)        { return m2_actv_speed; }
float motor2_get_actv_limit1(void)       { return m2_actv_limit1; }
float motor2_get_actv_limit2(void)       { return m2_actv_limit2; }
float motor2_get_actv_pause(void)        { return m2_actv_pause; }
float motor2_get_actv_fmax(void)         { return m2_actv_fmax; }
float motor2_get_actv_stiffness(void)    { return m2_actv_stiffness; }
