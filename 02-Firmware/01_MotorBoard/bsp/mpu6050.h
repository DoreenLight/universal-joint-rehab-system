#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef struct
{
  float accel_g[3];
  float gyro_dps[3];
  float euler_deg[3]; /* roll, pitch, yaw */
  float temperature_c;
  uint32_t sample_count;
  uint32_t i2c_error_count;
} MPU6050_DataTypeDef;

typedef enum
{
  MPU6050_INIT_ERROR_NONE = 0,
  MPU6050_INIT_ERROR_NO_ACK,
  MPU6050_INIT_ERROR_WHO_AM_I_READ,
  MPU6050_INIT_ERROR_WHO_AM_I_VALUE,
  MPU6050_INIT_ERROR_RESET,
  MPU6050_INIT_ERROR_CONFIG,
  MPU6050_INIT_ERROR_CALIBRATION
} MPU6050_InitErrorTypeDef;

/* Filtered, offset-corrected values kept for compatibility with existing code. */
extern volatile float ax;
extern volatile float ay;
extern volatile float az;
extern volatile float gx;
extern volatile float gy;
extern volatile float gz;
extern volatile float Temperature;
extern volatile float roll;
extern volatile float pitch;
extern volatile float yaw;

HAL_StatusTypeDef MPU6050_Init(void);
HAL_StatusTypeDef MPU6050_Calibrate(uint16_t sample_count);
HAL_StatusTypeDef MPU6050_RequestUpdate(void);
void MPU6050_Process(void);
uint8_t MPU6050_GetData(MPU6050_DataTypeDef *data);
void MPU6050_PrintUSART1(void);
MPU6050_InitErrorTypeDef MPU6050_GetInitError(void);
const char *MPU6050_GetInitErrorString(void);
HAL_StatusTypeDef MPU6050_GetLastBusStatus(void);
uint8_t MPU6050_GetDetectedAddress(void);
uint8_t MPU6050_GetWhoAmI(void);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
