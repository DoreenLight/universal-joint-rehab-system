#include "i2c.h"
#include "delay.h"

/*
 * PA2/PA3 do not have a hardware I2C alternate function on STM32F407.
 * They are therefore driven as open-drain GPIO according to the PCB wiring:
 *   PA2 -> IIC_SCK (SCL)
 *   PA3 -> IIC_SDA
 */

#define SOFT_I2C_DELAY_US       4U
#define SOFT_I2C_LINE_TIMEOUT   1000U

static void SoftI2C_Delay(void)
{
  delay_us(SOFT_I2C_DELAY_US);
}

static void SoftI2C_SclLow(void)
{
  HAL_GPIO_WritePin(SOFT_I2C_SCL_GPIO_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET);
}

static void SoftI2C_SclRelease(void)
{
  HAL_GPIO_WritePin(SOFT_I2C_SCL_GPIO_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET);
}

static void SoftI2C_SdaLow(void)
{
  HAL_GPIO_WritePin(SOFT_I2C_SDA_GPIO_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET);
}

static void SoftI2C_SdaRelease(void)
{
  HAL_GPIO_WritePin(SOFT_I2C_SDA_GPIO_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET);
}

static GPIO_PinState SoftI2C_ReadSda(void)
{
  return HAL_GPIO_ReadPin(SOFT_I2C_SDA_GPIO_PORT, SOFT_I2C_SDA_PIN);
}

static HAL_StatusTypeDef SoftI2C_WaitSclHigh(void)
{
  uint32_t timeout = SOFT_I2C_LINE_TIMEOUT;

  SoftI2C_SclRelease();
  while ((HAL_GPIO_ReadPin(SOFT_I2C_SCL_GPIO_PORT, SOFT_I2C_SCL_PIN) == GPIO_PIN_RESET) &&
         (timeout > 0U))
  {
    timeout--;
  }
  return (timeout > 0U) ? HAL_OK : HAL_TIMEOUT;
}

static HAL_StatusTypeDef SoftI2C_Start(void)
{
  SoftI2C_SdaRelease();
  if (SoftI2C_WaitSclHigh() != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  SoftI2C_Delay();
  if (SoftI2C_ReadSda() == GPIO_PIN_RESET)
  {
    return HAL_BUSY;
  }
  SoftI2C_SdaLow();
  SoftI2C_Delay();
  SoftI2C_SclLow();
  SoftI2C_Delay();
  return HAL_OK;
}

static void SoftI2C_Stop(void)
{
  SoftI2C_SdaLow();
  SoftI2C_Delay();
  (void)SoftI2C_WaitSclHigh();
  SoftI2C_Delay();
  SoftI2C_SdaRelease();
  SoftI2C_Delay();
}

static HAL_StatusTypeDef SoftI2C_WriteByte(uint8_t value)
{
  uint8_t bit;

  for (bit = 0U; bit < 8U; bit++)
  {
    if ((value & 0x80U) != 0U)
    {
      SoftI2C_SdaRelease();
    }
    else
    {
      SoftI2C_SdaLow();
    }
    SoftI2C_Delay();
    if (SoftI2C_WaitSclHigh() != HAL_OK)
    {
      SoftI2C_SclLow();
      return HAL_TIMEOUT;
    }
    SoftI2C_Delay();
    SoftI2C_SclLow();
    value <<= 1;
  }

  /* Ninth clock: release SDA and sample the slave ACK. */
  SoftI2C_SdaRelease();
  SoftI2C_Delay();
  if (SoftI2C_WaitSclHigh() != HAL_OK)
  {
    SoftI2C_SclLow();
    return HAL_TIMEOUT;
  }
  SoftI2C_Delay();
  bit = (uint8_t)SoftI2C_ReadSda();
  SoftI2C_SclLow();
  SoftI2C_Delay();
  return (bit == (uint8_t)GPIO_PIN_RESET) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef SoftI2C_ReadByte(uint8_t *value, uint8_t send_ack)
{
  uint8_t bit;
  uint8_t result = 0U;

  SoftI2C_SdaRelease();
  for (bit = 0U; bit < 8U; bit++)
  {
    result <<= 1;
    SoftI2C_Delay();
    if (SoftI2C_WaitSclHigh() != HAL_OK)
    {
      SoftI2C_SclLow();
      return HAL_TIMEOUT;
    }
    SoftI2C_Delay();
    if (SoftI2C_ReadSda() == GPIO_PIN_SET)
    {
      result |= 0x01U;
    }
    SoftI2C_SclLow();
  }

  if (send_ack != 0U)
  {
    SoftI2C_SdaLow();
  }
  else
  {
    SoftI2C_SdaRelease();
  }
  SoftI2C_Delay();
  if (SoftI2C_WaitSclHigh() != HAL_OK)
  {
    SoftI2C_SclLow();
    SoftI2C_SdaRelease();
    return HAL_TIMEOUT;
  }
  SoftI2C_Delay();
  SoftI2C_SclLow();
  SoftI2C_SdaRelease();
  SoftI2C_Delay();

  *value = result;
  return HAL_OK;
}

static void SoftI2C_RecoverBus(void)
{
  uint8_t pulse;

  SoftI2C_SdaRelease();
  SoftI2C_SclRelease();
  SoftI2C_Delay();
  if (SoftI2C_ReadSda() == GPIO_PIN_RESET)
  {
    for (pulse = 0U; pulse < 9U; pulse++)
    {
      SoftI2C_SclLow();
      SoftI2C_Delay();
      SoftI2C_SclRelease();
      SoftI2C_Delay();
    }
  }
  SoftI2C_Stop();
}

void MX_SoftI2C_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  delay_init();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Write high first so open-drain outputs start in the released state. */
  HAL_GPIO_WritePin(GPIOA, SOFT_I2C_SCL_PIN | SOFT_I2C_SDA_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = SOFT_I2C_SCL_PIN | SOFT_I2C_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  SoftI2C_RecoverBus();
}

void SoftI2C_GetBusState(uint8_t *scl_high, uint8_t *sda_high)
{
  SoftI2C_SclRelease();
  SoftI2C_SdaRelease();
  SoftI2C_Delay();
  if (scl_high != NULL)
  {
    *scl_high = (uint8_t)(HAL_GPIO_ReadPin(SOFT_I2C_SCL_GPIO_PORT,
                                           SOFT_I2C_SCL_PIN) == GPIO_PIN_SET);
  }
  if (sda_high != NULL)
  {
    *sda_high = (uint8_t)(HAL_GPIO_ReadPin(SOFT_I2C_SDA_GPIO_PORT,
                                           SOFT_I2C_SDA_PIN) == GPIO_PIN_SET);
  }
}

HAL_StatusTypeDef SoftI2C_IsDeviceReady(uint8_t address_7bit)
{
  HAL_StatusTypeDef status;

  SoftI2C_RecoverBus();
  status = SoftI2C_Start();
  if (status == HAL_OK)
  {
    status = SoftI2C_WriteByte((uint8_t)(address_7bit << 1));
  }
  SoftI2C_Stop();
  return status;
}

HAL_StatusTypeDef SoftI2C_MemWrite(uint8_t address_7bit, uint8_t reg,
                                   const uint8_t *data, uint16_t size)
{
  HAL_StatusTypeDef status;
  uint16_t i;

  if ((data == NULL) || (size == 0U))
  {
    return HAL_ERROR;
  }

  status = SoftI2C_Start();
  if (status == HAL_OK)
  {
    status = SoftI2C_WriteByte((uint8_t)(address_7bit << 1));
  }
  if (status == HAL_OK)
  {
    status = SoftI2C_WriteByte(reg);
  }
  for (i = 0U; (i < size) && (status == HAL_OK); i++)
  {
    status = SoftI2C_WriteByte(data[i]);
  }
  SoftI2C_Stop();
  return status;
}

HAL_StatusTypeDef SoftI2C_MemRead(uint8_t address_7bit, uint8_t reg,
                                  uint8_t *data, uint16_t size)
{
  HAL_StatusTypeDef status;
  uint16_t i;

  if ((data == NULL) || (size == 0U))
  {
    return HAL_ERROR;
  }

  status = SoftI2C_Start();
  if (status == HAL_OK)
  {
    status = SoftI2C_WriteByte((uint8_t)(address_7bit << 1));
  }
  if (status == HAL_OK)
  {
    status = SoftI2C_WriteByte(reg);
  }
  if (status == HAL_OK)
  {
    status = SoftI2C_Start();
  }
  if (status == HAL_OK)
  {
    status = SoftI2C_WriteByte((uint8_t)((address_7bit << 1) | 0x01U));
  }
  for (i = 0U; (i < size) && (status == HAL_OK); i++)
  {
    status = SoftI2C_ReadByte(&data[i], (uint8_t)(i + 1U < size));
  }
  SoftI2C_Stop();
  return status;
}
