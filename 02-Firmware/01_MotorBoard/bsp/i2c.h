#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* Software I2C pins fixed by the custom PCB schematic. */
#define SOFT_I2C_SCL_GPIO_PORT GPIOA
#define SOFT_I2C_SCL_PIN       GPIO_PIN_2
#define SOFT_I2C_SDA_GPIO_PORT GPIOA
#define SOFT_I2C_SDA_PIN       GPIO_PIN_3

void MX_SoftI2C_Init(void);
void SoftI2C_GetBusState(uint8_t *scl_high, uint8_t *sda_high);
HAL_StatusTypeDef SoftI2C_IsDeviceReady(uint8_t address_7bit);
HAL_StatusTypeDef SoftI2C_MemWrite(uint8_t address_7bit, uint8_t reg,
                                   const uint8_t *data, uint16_t size);
HAL_StatusTypeDef SoftI2C_MemRead(uint8_t address_7bit, uint8_t reg,
                                  uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */
