/**
  ******************************************************************************
  * @file    motor_ctrl.h
  * @brief   Motor control operations (shared by UART2 and UART3)
  *
  * Motor1 = PID-controlled DC motor (TIM2 PWM)
  * Motor2 = RS00 CAN motor
  ******************************************************************************
  */

#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include <stdint.h>

/* ============================ Motor1 (PID) ============================ */

void motor1_start(void);
void motor1_stop(void);
void motor1_set_angle(float angle);
void motor1_set_speed(float speed);
void motor1_set_current(float current);

/* Motor1 Kp/Ki/max_speed: flash-backed */
void motor1_set_kp(float kp);
void motor1_set_ki(float ki);
void motor1_set_max_speed(float spd);
float motor1_get_max_speed(void);

/* Mode: 0=dingdian(PID), 1=zhengxian, 2=fangbo, 3=PWM_test, 4=gengxin_flash */
void motor1_ctrl_set_mode(uint8_t mode);
uint8_t motor1_ctrl_get_mode(void);

/* PWM duty (-1.0~+1.0), sign = direction, used in PWM_test mode */
void motor1_set_pwm_duty(float duty);
float motor1_get_pwm_duty(void);

/* Max speed (deg/cycle), loaded from flash on boot */
extern float g_m1_max_speed;
extern float g_m2_max_speed;

/* Rehab passive control params */
void motor1_set_rehab_min(float angle);
void motor1_set_rehab_max(float angle);
void motor1_set_rehab_cycles(float cycles);
float motor1_get_rehab_min(void);
float motor1_get_rehab_max(void);
uint16_t motor1_get_rehab_cycles(void);

/* Active rehab params */
void motor1_set_actv_speed(float spd);
void motor1_set_actv_limit1(float angle);
void motor1_set_actv_limit2(float angle);
void motor1_set_actv_pause(float sec);
void motor1_set_actv_tmax(float tmax);
void motor1_set_actv_stiffness(float k);
float motor1_get_actv_speed(void);
float motor1_get_actv_limit1(void);
float motor1_get_actv_limit2(void);
float motor1_get_actv_pause(void);
float motor1_get_actv_tmax(void);
float motor1_get_actv_stiffness(void);

/* M2 Active rehab params */
void motor2_set_actv_speed(float spd);
void motor2_set_actv_limit1(float angle);
void motor2_set_actv_limit2(float angle);
void motor2_set_actv_pause(float sec);
void motor2_set_actv_fmax(float fmax);
void motor2_set_actv_stiffness(float k);
float motor2_get_actv_speed(void);
float motor2_get_actv_limit1(void);
float motor2_get_actv_limit2(void);
float motor2_get_actv_pause(void);
float motor2_get_actv_fmax(void);
float motor2_get_actv_stiffness(void);
void motor2_set_rehab_min(float angle);
void motor2_set_rehab_max(float angle);
void motor2_set_rehab_cycles(float cycles);
float motor2_get_rehab_min(void);
float motor2_get_rehab_max(void);
uint16_t motor2_get_rehab_cycles(void);

/* ============================ Motor2 (RS00 CAN) ============================ */

/* IO commands */
void motor2_enable(void);
void motor2_stop(void);
void motor2_set_mode(uint8_t mode);
void motor2_set_zero(void);
void motor2_clear_error(void);

/* Float parameters — each update sends RS00_Control with all cached values */
void motor2_set_position(float pos);
void motor2_set_velocity(float vel);
void motor2_set_torque(float torque);
void motor2_set_kp(float kp);              /* also cached for flash */
void motor2_set_kd(float kd);              /* also cached for flash */
void motor2_set_max_speed(float spd);      /* also cached for flash */
void motor2_set_current_limit(float limit);

/* Getters for trajectory planner */
float motor2_get_position(void);
float motor2_get_velocity(void);
float motor2_get_torque(void);
float motor2_get_kp(void);
float motor2_get_kd(void);
float motor2_get_max_speed(void);

/* Called from control loop (was private, now public for Order_Creat_process) */
void motor2_send_control(float pos, float vel, float torque, float kp, float kd);

#endif /* MOTOR_CTRL_H */
