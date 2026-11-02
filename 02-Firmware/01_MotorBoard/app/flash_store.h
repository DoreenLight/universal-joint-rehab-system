/**
  ******************************************************************************
  * @file    flash_store.h
  * @brief   Flash parameter storage (PID values for both motors)
  ******************************************************************************
  */

#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

/* --- Parameter struct stored in Flash --- */
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;         /* 0x4C473031 = "LG01" */
    float    motor1_kp;
    float    motor1_ki;
    float    motor1_max_speed;
    float    motor2_kp;
    float    motor2_kd;
    float    motor2_max_speed;
    uint32_t checksum;      /* XOR of all bytes after magic through before checksum */
} flash_params_t;
#pragma pack(pop)

#define FLASH_PARAMS_MAGIC  0x4C473031u  /* "LG01" */

/* --- Public API --- */
void flash_store_load(void);   /* Read params from Flash, apply to variables */
void flash_store_save(void);   /* Write cached params to Flash AND apply to runtime */
uint8_t flash_store_is_valid(void);  /* Check if stored params on Flash are valid */
uint8_t flash_store_set_param(uint8_t id, float value);  /* Cache a value (no Flash write) */
uint8_t flash_store_is_modified(void);  /* Check if cache has unsaved changes */

#endif /* FLASH_STORE_H */
