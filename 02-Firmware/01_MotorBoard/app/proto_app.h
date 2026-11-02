/**
  ******************************************************************************
  * @file    proto_app.h
  * @brief   UART3 Application Layer: IO signals and float values
  ******************************************************************************
  */

#ifndef PROTO_APP_H
#define PROTO_APP_H

#include <stdint.h>

/* --- Max floats per telemetry frame --- */
#define PROTO_TELEM_MAX_FLOATS  18u

/* --- Float value IDs (MCU->Host telemetry, Host->MCU write) --- */
#define PROTO_FLOAT_ID_MOTOR1_ANGLE    0x01u
#define PROTO_FLOAT_ID_MOTOR1_SPEED    0x02u
#define PROTO_FLOAT_ID_MOTOR1_CURRENT  0x03u
#define PROTO_FLOAT_ID_MOTOR1_KP       0x04u
#define PROTO_FLOAT_ID_MOTOR1_KI       0x05u
#define PROTO_FLOAT_ID_MOTOR1_PWM      0x06u   /* PWM duty (-1.0 ~ +1.0), PWM_test only */
#define PROTO_FLOAT_ID_MOTOR1_MAX_SPEED 0x07u  /* trajectory max speed (deg/cycle) */
#define PROTO_FLOAT_ID_MOTOR1_REHAB_MIN  0x08u   /* rehab min angle (°) */
#define PROTO_FLOAT_ID_MOTOR1_REHAB_MAX  0x09u   /* rehab max angle (°) */
#define PROTO_FLOAT_ID_MOTOR1_REHAB_CYCLES 0x0Au /* rehab cycles */
#define PROTO_FLOAT_ID_M1_ACTV_SPEED   0x0Bu   /* active rehab T-wave speed (°/s) */
#define PROTO_FLOAT_ID_M1_ACTV_LIMIT1  0x0Cu   /* active rehab angle limit 1 (°) */
#define PROTO_FLOAT_ID_M1_ACTV_LIMIT2  0x0Du   /* active rehab angle limit 2 (°) */
#define PROTO_FLOAT_ID_M1_ACTV_PAUSE   0x0Eu   /* active rehab pause time (s) */
#define PROTO_FLOAT_ID_M1_ACTV_TMAX    0x0Fu   /* active rehab max torque (N·m) */
#define PROTO_FLOAT_ID_M1_ACTV_STIFF   0x10u   /* active rehab stiffness k */
#define PROTO_FLOAT_ID_MOTOR2_ANGLE    0x11u
#define PROTO_FLOAT_ID_MOTOR2_SPEED    0x12u
#define PROTO_FLOAT_ID_MOTOR2_TORQUE   0x13u
#define PROTO_FLOAT_ID_MOTOR2_KP       0x14u
#define PROTO_FLOAT_ID_MOTOR2_KD            0x15u
#define PROTO_FLOAT_ID_MOTOR2_POS_ACTUAL  0x16u   /* M2 actual position feedback (°) */
#define PROTO_FLOAT_ID_MOTOR2_CURRENT_LIMIT 0x17u
#define PROTO_FLOAT_ID_MOTOR2_MAX_SPEED  0x18u  /* trajectory max speed (deg/cycle) */
#define PROTO_FLOAT_ID_MOTOR2_VEL_ACTUAL  0x19u   /* M2 actual velocity feedback (°/s) */
#define PROTO_FLOAT_ID_MOTOR2_REHAB_MIN  0x1Au   /* M2 rehab min angle (°) */
#define PROTO_FLOAT_ID_MOTOR2_REHAB_MAX  0x1Bu   /* M2 rehab max angle (°) */
#define PROTO_FLOAT_ID_MOTOR2_REHAB_CYCLES 0x1Cu /* M2 rehab cycles */
#define PROTO_FLOAT_ID_M2_ACTV_SPEED   0x1Du   /* M2 active rehab T-wave speed */
#define PROTO_FLOAT_ID_M2_ACTV_LIMIT1  0x1Eu   /* M2 active rehab limit1 */
#define PROTO_FLOAT_ID_M2_ACTV_LIMIT2  0x1Fu   /* M2 active rehab limit2 */
#define PROTO_FLOAT_ID_M2_ACTV_PAUSE   0x20u   /* M2 active rehab pause time */
#define PROTO_FLOAT_ID_M2_ACTV_FMAX    0x23u   /* M2 active rehab force threshold */
#define PROTO_FLOAT_ID_M2_ACTV_STIFF   0x24u   /* M2 active rehab stiffness k */
#define PROTO_FLOAT_ID_SENSOR_ANGLE    0x21u
#define PROTO_FLOAT_ID_SENSOR_CURRENT  0x22u
#define PROTO_FLOAT_ID_IMU_ROLL        0x31u
#define PROTO_FLOAT_ID_IMU_PITCH       0x32u
#define PROTO_FLOAT_ID_IMU_YAW         0x33u
#define PROTO_FLOAT_ID_FORCE_FX       0x41u   /* DR304: Fx (N) */
#define PROTO_FLOAT_ID_FORCE_FY       0x42u   /* DR304: Fy (N) */
#define PROTO_FLOAT_ID_FORCE_FZ       0x43u   /* DR304: Fz (N) */
#define PROTO_FLOAT_ID_FORCE_MX       0x44u   /* DR304: Mx (N·m) */
#define PROTO_FLOAT_ID_FORCE_MY       0x45u   /* DR304: My (N·m) */
#define PROTO_FLOAT_ID_FORCE_MZ       0x46u   /* DR304: Mz (N·m) */

/* --- IO signal IDs --- */
#define PROTO_IO_ID_M1_START_STOP      0x01u   /* Motor1: 1=start, 2=stop */
#define PROTO_IO_ID_MODE               0x02u   /* System: 0=dingdian,1=zhengxian,2=fangbo,3=PWM_test,4=flash */
#define PROTO_IO_ID_M2_START_STOP      0x03u   /* Motor2: 1=enable, 2=stop */
#define PROTO_IO_ID_M2_RUN_MODE        0x04u   /* RS00: 1=运控,2=位置,3=速度,5=CSP */
#define PROTO_IO_ID_M2_SET_ZERO        0x05u   /* Motor2: set mechanical zero */
#define PROTO_IO_ID_M2_CLEAR_ERR       0x06u   /* Motor2: clear error */
#define PROTO_IO_ID_M2_TRAJ_MODE       0x07u   /* Motor2: 0=position, 1=velocity */
#define PROTO_IO_ID_M1_REHAB           0x08u   /* Motor1 rehab: 1=start, 2=stop */
#define PROTO_IO_ID_M2_REHAB           0x09u   /* Motor2 rehab: 1=start, 2=stop */
#define PROTO_IO_ID_SYS_RUN            0x0Au   /* System: 1=start TIM2 loop, 2=stop */
#define PROTO_IO_ID_M1_ACTV_REHAB      0x0Bu   /* M1 active rehab: 1=start, 2=stop */
#define PROTO_IO_ID_FORCE_ZERO         0x0Cu   /* Force sensor zero calibration */
#define PROTO_IO_ID_M2_ACTV_REHAB      0x0Du   /* M2 active rehab: 1=start, 2=stop */
#define PROTO_IO_ID_M2_ACTV_MODE       0x0Eu   /* M2 active rehab mode: 1=flex/ext, 2=ulnar/radial */

/* --- Application-layer CMD bytes ---
   (transport-level PING=0x84/ACK=0x83 defined in proto_transport.h) --- */
#define PROTO_CMD_IO_WRITE      0x01u   /* Host->MCU: [id:1B][val:1B] */
#define PROTO_CMD_FLOAT_WRITE   0x02u   /* Host->MCU: [id:1B][val:4B_LE] */
#define PROTO_CMD_BATCH_WRITE   0x03u   /* Host->MCU: [cnt:1B][(id:1B][val:4B_LE)]* */
#define PROTO_CMD_TELEMETRY     0x81u   /* MCU->Host: [cnt:1B][(id:1B][val:4B_LE)]* */
#define PROTO_CMD_IO_REPORT     0x82u   /* MCU->Host: [id:1B][val:1B] */
#define PROTO_CMD_FLASH_SAVE    0x85u   /* Host->MCU: save current params to Flash */

/* --- Application callbacks --- */
typedef void (*proto_app_float_cb_t)(uint8_t float_id, float value);
typedef void (*proto_app_io_cb_t)(uint8_t io_id, uint8_t state);

/* --- Public API --- */
void proto_app_init(void);

/* Send one float (convenience, calls proto_transport_send) */
uint8_t proto_app_send_float(uint8_t float_id, float value);

/* Send multiple floats in a single frame (main use case for telemetry) */
uint8_t proto_app_send_telemetry(uint8_t count,
                                 const uint8_t *float_ids,
                                 const float *values);

/* Send IO signal */
uint8_t proto_app_send_io(uint8_t io_id, uint8_t state);

/* Register callbacks */
void proto_app_set_float_callback(proto_app_float_cb_t cb);
void proto_app_set_io_callback(proto_app_io_cb_t cb);

/* Called by transport layer when a valid frame arrives */
void proto_app_handle_frame(uint8_t cmd, const uint8_t *payload, uint8_t len);

#endif /* PROTO_APP_H */
