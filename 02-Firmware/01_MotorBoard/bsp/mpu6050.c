#include "mpu6050.h"
#include "i2c.h"
#include "usart.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define MPU6050_ADDRESS_LOW          0x68U
#define MPU6050_ADDRESS_HIGH         0x69U
#define MPU6050_REG_SMPLRT_DIV       0x19U
#define MPU6050_REG_CONFIG           0x1AU
#define MPU6050_REG_GYRO_CONFIG      0x1BU
#define MPU6050_REG_ACCEL_CONFIG     0x1CU
#define MPU6050_REG_ACCEL_XOUT_H     0x3BU
#define MPU6050_REG_PWR_MGMT_1       0x6BU
#define MPU6050_REG_WHO_AM_I         0x75U

#define MPU6050_WHO_AM_I_VALUE       0x68U
#define MPU6500_WHO_AM_I_VALUE       0x70U
#define MPU6050_FRAME_SIZE           14U
#define MPU6050_ACCEL_SCALE          (1.0f / 16384.0f) /* +/-2 g */
#define MPU6050_GYRO_SCALE           (1.0f / 65.5f)    /* +/-500 dps */
#define MPU6050_FILTER_ALPHA         0.20f
#define MPU6050_GYRO_DEADBAND_DPS    0.50f
#define MPU6050_ACCEL_DEADBAND_G     0.010f
#define MPU6050_BIAS_TRACK_ALPHA     0.0005f
#define MPU6050_COMPLEMENTARY_ALPHA  0.98f
#define MPU6050_RAD_TO_DEG           57.2957795f

volatile float ax = 0.0f;
volatile float ay = 0.0f;
volatile float az = 0.0f;
volatile float gx = 0.0f;
volatile float gy = 0.0f;
volatile float gz = 0.0f;
volatile float Temperature = 0.0f;
volatile float roll = 0.0f;
volatile float pitch = 0.0f;
volatile float yaw = 0.0f;

static uint8_t mpu_rx_buf[MPU6050_FRAME_SIZE];
static volatile uint8_t mpu_raw_ready = 0U;
static volatile uint8_t mpu_data_ready = 0U;
static volatile uint32_t mpu_sample_count = 0U;
static volatile uint32_t mpu_i2c_error_count = 0U;
static float accel_bias[3] = {0.0f, 0.0f, 0.0f};
static float gyro_bias[3] = {0.0f, 0.0f, 0.0f};
static float accel_filter[3] = {0.0f, 0.0f, 0.0f};
static float gyro_filter[3] = {0.0f, 0.0f, 0.0f};
static float filtered_temperature = 0.0f;
static uint8_t filter_initialized = 0U;
static uint8_t attitude_initialized = 0U;
static uint32_t attitude_last_tick = 0U;
static uint8_t mpu_address_7bit = MPU6050_ADDRESS_LOW;
static uint8_t mpu_who_am_i = 0U;
static MPU6050_InitErrorTypeDef mpu_init_error = MPU6050_INIT_ERROR_NONE;
static HAL_StatusTypeDef mpu_last_bus_status = HAL_OK;

static HAL_StatusTypeDef MPU6050_WriteRegister(uint8_t reg, uint8_t value)
{
  mpu_last_bus_status = SoftI2C_MemWrite(mpu_address_7bit, reg, &value, 1U);
  return mpu_last_bus_status;
}

static HAL_StatusTypeDef MPU6050_ReadRegisters(uint8_t reg, uint8_t *data, uint16_t size)
{
  mpu_last_bus_status = SoftI2C_MemRead(mpu_address_7bit, reg, data, size);
  return mpu_last_bus_status;
}

static int16_t MPU6050_MakeInt16(uint8_t high, uint8_t low)
{
  return (int16_t)(((uint16_t)high << 8) | low);
}

static float MPU6050_ApplyDeadband(float value, float centre, float deadband)
{
  if (fabsf(value - centre) < deadband)
  {
    return centre;
  }
  return value;
}

static float MPU6050_WrapAngle(float angle_deg)
{
  while (angle_deg > 180.0f)
  {
    angle_deg -= 360.0f;
  }
  while (angle_deg < -180.0f)
  {
    angle_deg += 360.0f;
  }
  return angle_deg;
}

HAL_StatusTypeDef MPU6050_Init(void)
{
  static const uint8_t addresses[2] = {MPU6050_ADDRESS_LOW, MPU6050_ADDRESS_HIGH};
  uint8_t address_index;
  uint8_t trial;
  uint8_t detected = 0U;

  mpu_init_error = MPU6050_INIT_ERROR_NONE;
  mpu_who_am_i = 0U;
  HAL_Delay(200U);

  for (address_index = 0U; address_index < 2U; address_index++)
  {
    mpu_address_7bit = addresses[address_index];
    for (trial = 0U; trial < 5U; trial++)
    {
      mpu_last_bus_status = SoftI2C_IsDeviceReady(mpu_address_7bit);
      if (mpu_last_bus_status == HAL_OK)
      {
        detected = 1U;
        break;
      }
      HAL_Delay(10U);
    }
    if (detected != 0U)
    {
      break;
    }
  }

  if (detected == 0U)
  {
    mpu_init_error = MPU6050_INIT_ERROR_NO_ACK;
    return HAL_ERROR;
  }

  if (MPU6050_ReadRegisters(MPU6050_REG_WHO_AM_I, &mpu_who_am_i, 1U) != HAL_OK)
  {
    mpu_init_error = MPU6050_INIT_ERROR_WHO_AM_I_READ;
    return HAL_ERROR;
  }
  if ((mpu_who_am_i != MPU6050_WHO_AM_I_VALUE) &&
      (mpu_who_am_i != MPU6500_WHO_AM_I_VALUE))
  {
    mpu_init_error = MPU6050_INIT_ERROR_WHO_AM_I_VALUE;
    return HAL_ERROR;
  }

  if (MPU6050_WriteRegister(MPU6050_REG_PWR_MGMT_1, 0x80U) != HAL_OK)
  {
    mpu_init_error = MPU6050_INIT_ERROR_RESET;
    return HAL_ERROR;
  }
  HAL_Delay(100U);

  /* PLL with X gyro clock, 200 Hz sample rate, DLPF enabled. */
  if ((MPU6050_WriteRegister(MPU6050_REG_PWR_MGMT_1, 0x01U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_CONFIG, 0x03U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_SMPLRT_DIV, 0x04U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_GYRO_CONFIG, 0x08U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_ACCEL_CONFIG, 0x00U) != HAL_OK))
  {
    mpu_init_error = MPU6050_INIT_ERROR_CONFIG;
    return HAL_ERROR;
  }

  HAL_Delay(50U);
  return HAL_OK;
}

HAL_StatusTypeDef MPU6050_Calibrate(uint16_t sample_count)
{
  uint8_t raw[MPU6050_FRAME_SIZE];
  float accel_sum[3] = {0.0f, 0.0f, 0.0f};
  float gyro_sum[3] = {0.0f, 0.0f, 0.0f};
  uint16_t i;

  if (sample_count == 0U)
  {
    return HAL_ERROR;
  }

  for (i = 0U; i < sample_count; i++)
  {
    if (MPU6050_ReadRegisters(MPU6050_REG_ACCEL_XOUT_H,
                              raw, MPU6050_FRAME_SIZE) != HAL_OK)
    {
      mpu_init_error = MPU6050_INIT_ERROR_CALIBRATION;
      return HAL_ERROR;
    }

    accel_sum[0] += (float)MPU6050_MakeInt16(raw[0], raw[1]) * MPU6050_ACCEL_SCALE;
    accel_sum[1] += (float)MPU6050_MakeInt16(raw[2], raw[3]) * MPU6050_ACCEL_SCALE;
    accel_sum[2] += (float)MPU6050_MakeInt16(raw[4], raw[5]) * MPU6050_ACCEL_SCALE;
    gyro_sum[0] += (float)MPU6050_MakeInt16(raw[8], raw[9]) * MPU6050_GYRO_SCALE;
    gyro_sum[1] += (float)MPU6050_MakeInt16(raw[10], raw[11]) * MPU6050_GYRO_SCALE;
    gyro_sum[2] += (float)MPU6050_MakeInt16(raw[12], raw[13]) * MPU6050_GYRO_SCALE;
    HAL_Delay(5U);
  }

  accel_bias[0] = accel_sum[0] / (float)sample_count;
  accel_bias[1] = accel_sum[1] / (float)sample_count;
  /* Remove the complete stationary gravity vector so all axes read zero. */
  accel_bias[2] = accel_sum[2] / (float)sample_count;
  gyro_bias[0] = gyro_sum[0] / (float)sample_count;
  gyro_bias[1] = gyro_sum[1] / (float)sample_count;
  gyro_bias[2] = gyro_sum[2] / (float)sample_count;

  filter_initialized = 0U;
  attitude_initialized = 0U;
  attitude_last_tick = 0U;
  roll = 0.0f;
  pitch = 0.0f;
  yaw = 0.0f;
  mpu_sample_count = 0U;
  mpu_i2c_error_count = 0U;
  mpu_init_error = MPU6050_INIT_ERROR_NONE;
  return HAL_OK;
}

MPU6050_InitErrorTypeDef MPU6050_GetInitError(void)
{
  return mpu_init_error;
}

const char *MPU6050_GetInitErrorString(void)
{
  switch (mpu_init_error)
  {
    case MPU6050_INIT_ERROR_NONE:          return "none";
    case MPU6050_INIT_ERROR_NO_ACK:        return "no ACK at 0x68/0x69";
    case MPU6050_INIT_ERROR_WHO_AM_I_READ: return "WHO_AM_I read failed";
    case MPU6050_INIT_ERROR_WHO_AM_I_VALUE:return "WHO_AM_I value mismatch";
    case MPU6050_INIT_ERROR_RESET:         return "reset write failed";
    case MPU6050_INIT_ERROR_CONFIG:        return "configuration write failed";
    case MPU6050_INIT_ERROR_CALIBRATION:   return "calibration read failed";
    default:                               return "unknown";
  }
}

HAL_StatusTypeDef MPU6050_GetLastBusStatus(void)
{
  return mpu_last_bus_status;
}

uint8_t MPU6050_GetDetectedAddress(void)
{
  return mpu_address_7bit;
}

uint8_t MPU6050_GetWhoAmI(void)
{
  return mpu_who_am_i;
}

HAL_StatusTypeDef MPU6050_RequestUpdate(void)
{
  HAL_StatusTypeDef status;

  if (mpu_raw_ready != 0U)
  {
    return HAL_BUSY;
  }

  status = MPU6050_ReadRegisters(MPU6050_REG_ACCEL_XOUT_H,
                                 mpu_rx_buf, MPU6050_FRAME_SIZE);
  if (status != HAL_OK)
  {
    mpu_i2c_error_count++;
  }
  else
  {
    mpu_raw_ready = 1U;
  }
  return status;
}

void MPU6050_Process(void)
{
  float accel_raw[3];
  float accel_attitude[3];
  float gyro_raw[3];
  float accel_magnitude;
  float accel_roll;
  float accel_pitch;
  float sample_period_s;
  uint32_t now_tick;
  uint8_t stationary;

  if (mpu_raw_ready == 0U)
  {
    return;
  }

  mpu_raw_ready = 0U;

  accel_raw[0] = (float)MPU6050_MakeInt16(mpu_rx_buf[0], mpu_rx_buf[1]) * MPU6050_ACCEL_SCALE;
  accel_raw[1] = (float)MPU6050_MakeInt16(mpu_rx_buf[2], mpu_rx_buf[3]) * MPU6050_ACCEL_SCALE;
  accel_raw[2] = (float)MPU6050_MakeInt16(mpu_rx_buf[4], mpu_rx_buf[5]) * MPU6050_ACCEL_SCALE;
  memcpy(accel_attitude, accel_raw, sizeof(accel_attitude));
  gyro_raw[0] = (float)MPU6050_MakeInt16(mpu_rx_buf[8], mpu_rx_buf[9]) * MPU6050_GYRO_SCALE;
  gyro_raw[1] = (float)MPU6050_MakeInt16(mpu_rx_buf[10], mpu_rx_buf[11]) * MPU6050_GYRO_SCALE;
  gyro_raw[2] = (float)MPU6050_MakeInt16(mpu_rx_buf[12], mpu_rx_buf[13]) * MPU6050_GYRO_SCALE;

  accel_magnitude = sqrtf(accel_raw[0] * accel_raw[0] +
                          accel_raw[1] * accel_raw[1] +
                          accel_raw[2] * accel_raw[2]);
  stationary = (uint8_t)((accel_magnitude > 0.98f) &&
                         (accel_magnitude < 1.02f) &&
                         (fabsf(gyro_raw[0] - gyro_bias[0]) < 2.0f) &&
                         (fabsf(gyro_raw[1] - gyro_bias[1]) < 2.0f) &&
                         (fabsf(gyro_raw[2] - gyro_bias[2]) < 2.0f));

  /* Slowly follow temperature-related gyro zero drift only while stationary. */
  if (stationary != 0U)
  {
    gyro_bias[0] += MPU6050_BIAS_TRACK_ALPHA * (gyro_raw[0] - gyro_bias[0]);
    gyro_bias[1] += MPU6050_BIAS_TRACK_ALPHA * (gyro_raw[1] - gyro_bias[1]);
    gyro_bias[2] += MPU6050_BIAS_TRACK_ALPHA * (gyro_raw[2] - gyro_bias[2]);
  }

  accel_raw[0] -= accel_bias[0];
  accel_raw[1] -= accel_bias[1];
  accel_raw[2] -= accel_bias[2];
  gyro_raw[0] -= gyro_bias[0];
  gyro_raw[1] -= gyro_bias[1];
  gyro_raw[2] -= gyro_bias[2];

  if (filter_initialized == 0U)
  {
    memcpy(accel_filter, accel_raw, sizeof(accel_filter));
    memcpy(gyro_filter, gyro_raw, sizeof(gyro_filter));
    filtered_temperature =
        (float)MPU6050_MakeInt16(mpu_rx_buf[6], mpu_rx_buf[7]) / 340.0f + 36.53f;
    filter_initialized = 1U;
  }
  else
  {
    uint8_t i;
    for (i = 0U; i < 3U; i++)
    {
      accel_filter[i] += MPU6050_FILTER_ALPHA * (accel_raw[i] - accel_filter[i]);
      gyro_filter[i] += MPU6050_FILTER_ALPHA * (gyro_raw[i] - gyro_filter[i]);
    }
    filtered_temperature += MPU6050_FILTER_ALPHA *
        (((float)MPU6050_MakeInt16(mpu_rx_buf[6], mpu_rx_buf[7]) / 340.0f + 36.53f) -
         filtered_temperature);
  }

  now_tick = HAL_GetTick();
  if (attitude_last_tick == 0U)
  {
    sample_period_s = 0.005f;
  }
  else
  {
    sample_period_s = (float)(uint32_t)(now_tick - attitude_last_tick) * 0.001f;
    if ((sample_period_s < 0.001f) || (sample_period_s > 0.050f))
    {
      sample_period_s = 0.005f;
    }
  }
  attitude_last_tick = now_tick;

  /* Standard convention: X=roll, Y=pitch, Z=yaw. */
  accel_roll = atan2f(accel_attitude[1], accel_attitude[2]) * MPU6050_RAD_TO_DEG;
  accel_pitch = atan2f(-accel_attitude[0],
                       sqrtf(accel_attitude[1] * accel_attitude[1] +
                             accel_attitude[2] * accel_attitude[2])) * MPU6050_RAD_TO_DEG;

  if (attitude_initialized == 0U)
  {
    roll = accel_roll;
    pitch = accel_pitch;
    yaw = 0.0f;
    attitude_initialized = 1U;
  }
  else
  {
    float roll_gyro = roll + gyro_filter[0] * sample_period_s;
    float pitch_gyro = pitch + gyro_filter[1] * sample_period_s;

    /* Reject accelerometer angle correction during strong linear acceleration. */
    if ((accel_magnitude > 0.80f) && (accel_magnitude < 1.20f))
    {
      roll = MPU6050_COMPLEMENTARY_ALPHA * roll_gyro +
             (1.0f - MPU6050_COMPLEMENTARY_ALPHA) * accel_roll;
      pitch = MPU6050_COMPLEMENTARY_ALPHA * pitch_gyro +
              (1.0f - MPU6050_COMPLEMENTARY_ALPHA) * accel_pitch;
    }
    else
    {
      roll = roll_gyro;
      pitch = pitch_gyro;
    }
    yaw = MPU6050_WrapAngle(yaw + gyro_filter[2] * sample_period_s);
    roll = MPU6050_WrapAngle(roll);
    pitch = MPU6050_WrapAngle(pitch);
  }

  ax = MPU6050_ApplyDeadband(accel_filter[0], 0.0f, MPU6050_ACCEL_DEADBAND_G);
  ay = MPU6050_ApplyDeadband(accel_filter[1], 0.0f, MPU6050_ACCEL_DEADBAND_G);
  az = MPU6050_ApplyDeadband(accel_filter[2], 0.0f, MPU6050_ACCEL_DEADBAND_G);
  gx = MPU6050_ApplyDeadband(gyro_filter[0], 0.0f, MPU6050_GYRO_DEADBAND_DPS);
  gy = MPU6050_ApplyDeadband(gyro_filter[1], 0.0f, MPU6050_GYRO_DEADBAND_DPS);
  gz = MPU6050_ApplyDeadband(gyro_filter[2], 0.0f, MPU6050_GYRO_DEADBAND_DPS);
  Temperature = filtered_temperature;
  mpu_sample_count++;
  mpu_data_ready = 1U;
}

uint8_t MPU6050_GetData(MPU6050_DataTypeDef *data)
{
  uint32_t primask;

  if ((data == NULL) || (mpu_data_ready == 0U))
  {
    return 0U;
  }

  primask = __get_PRIMASK();
  __disable_irq();
  data->accel_g[0] = ax;
  data->accel_g[1] = ay;
  data->accel_g[2] = az;
  data->gyro_dps[0] = gx;
  data->gyro_dps[1] = gy;
  data->gyro_dps[2] = gz;
  data->euler_deg[0] = roll;
  data->euler_deg[1] = pitch;
  data->euler_deg[2] = yaw;
  data->temperature_c = Temperature;
  data->sample_count = mpu_sample_count;
  data->i2c_error_count = mpu_i2c_error_count;
  mpu_data_ready = 0U;
  if (primask == 0U)
  {
    __enable_irq();
  }
  return 1U;
}

void MPU6050_PrintUSART1(void)
{
  MPU6050_DataTypeDef data;
  char text[224];
  int length;

  if (MPU6050_GetData(&data) == 0U)
  {
    return;
  }

  length = snprintf(text, sizeof(text),
                    "IMU A_zeroed[g] X:%+.4f Y:%+.4f Z:%+.4f  G[dps] X:%+.3f Y:%+.3f Z:%+.3f  Euler[deg] R:%+.2f P:%+.2f Y:%+.2f  err:%lu\r\n",
                    data.accel_g[0], data.accel_g[1], data.accel_g[2],
                    data.gyro_dps[0], data.gyro_dps[1], data.gyro_dps[2],
                    data.euler_deg[0], data.euler_deg[1], data.euler_deg[2],
                    (unsigned long)data.i2c_error_count);
  if (length > 0)
  {
    uint16_t tx_length = (length < (int)sizeof(text)) ? (uint16_t)length :
                                                         (uint16_t)(sizeof(text) - 1U);
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)text, tx_length, 100U);
  }
}
