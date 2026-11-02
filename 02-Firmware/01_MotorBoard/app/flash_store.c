/**
  ******************************************************************************
  * @file    flash_store.c
  * @brief   Flash parameter storage using HAL_FLASH (STM32F407VETx)
  *
  * Uses FLASH_SECTOR_7 (0x08060000, 128KB) for parameter storage.
  * Erases whole sector before write (infrequent operation).
  ******************************************************************************
  */

#include "flash_store.h"
#include "stm32f4xx_hal.h"
#include "controller.h"
#include "proto_app.h"
#include "motor_ctrl.h"
#include <stdio.h>
#include <string.h>

/* --- Flash address: last 128KB sector of 512KB F407 --- */
#define FLASH_PARAMS_ADDR    ((uint32_t)0x08060000u)
#define FLASH_PARAMS_SECTOR  FLASH_SECTOR_7

/* --- Static: cached copy and dirty flag --- */
static flash_params_t g_flash_params;
static uint8_t        g_flash_modified = 0u;

/* --- Simple XOR checksum --- */
static uint32_t flash_checksum(const flash_params_t *p)
{
    uint32_t sum = 0xAAAAAAAAu;
    const uint8_t *bytes = (const uint8_t *)&p->motor1_kp;
    uint32_t i;
    uint32_t field_len = (uint32_t)(sizeof(flash_params_t) -
                                    offsetof(flash_params_t, motor1_kp) -
                                    sizeof(uint32_t));

    for (i = 0u; i < field_len; i++)
    {
        sum ^= (uint32_t)bytes[i];
        sum = (sum << 1) | (sum >> 31);
    }
    return sum;
}

/* --- Public API --- */

uint8_t flash_store_is_valid(void)
{
    const flash_params_t *p = (const flash_params_t *)FLASH_PARAMS_ADDR;
    uint32_t calc;

    if (p->magic != FLASH_PARAMS_MAGIC)
    {
        return 0u;
    }

    calc = flash_checksum(p);
    if (calc != p->checksum)
    {
        return 0u;
    }

    return 1u;
}

void flash_store_load(void)
{
    const flash_params_t *p = (const flash_params_t *)FLASH_PARAMS_ADDR;

    if (!flash_store_is_valid())
    {
        printf("FLASH: no valid params, using defaults\r\n");
        return;
    }

    (void)memcpy(&g_flash_params, (const void *)p, sizeof(flash_params_t));

    /* Apply to runtime variables via motor_ctrl setters */
    motor1_set_kp(g_flash_params.motor1_kp);
    motor1_set_ki(g_flash_params.motor1_ki);
    motor2_set_kp(g_flash_params.motor2_kp);
    motor2_set_kd(g_flash_params.motor2_kd);

    /* Clear the dirty flag set by the setters above (we just loaded from Flash) */
    g_flash_modified = 0u;

    /* Restore max_speed (default 1.0 = 100 deg/s if flash never saved) */
    g_m1_max_speed = (g_flash_params.motor1_max_speed > 0.01f)
                     ? g_flash_params.motor1_max_speed : 1.0f;
    g_m2_max_speed = (g_flash_params.motor2_max_speed > 0.01f)
                     ? g_flash_params.motor2_max_speed : 1.0f;

    printf("FLASH: loaded Kp1=%.4f Ki1=%.4f Spd1=%.1f Kp2=%.4f Kd2=%.4f Spd2=%.1f\r\n",
           (double)g_flash_params.motor1_kp,
           (double)g_flash_params.motor1_ki,
           (double)g_flash_params.motor1_max_speed,
           (double)g_flash_params.motor2_kp,
           (double)g_flash_params.motor2_kd,
           (double)g_flash_params.motor2_max_speed);
}

/* --- Cache a float value by ID (no Flash write, no runtime apply) --- */
uint8_t flash_store_set_param(uint8_t id, float value)
{
    switch (id)
    {
    case PROTO_FLOAT_ID_MOTOR1_KP:
        g_flash_params.motor1_kp = value;
        break;
    case PROTO_FLOAT_ID_MOTOR1_KI:
        g_flash_params.motor1_ki = value;
        break;
    case PROTO_FLOAT_ID_MOTOR2_KP:
        g_flash_params.motor2_kp = value;
        break;
    case PROTO_FLOAT_ID_MOTOR2_KD:
        g_flash_params.motor2_kd = value;
        break;
    case PROTO_FLOAT_ID_MOTOR1_MAX_SPEED:
        g_flash_params.motor1_max_speed = value;
        break;
    case PROTO_FLOAT_ID_MOTOR2_MAX_SPEED:
        g_flash_params.motor2_max_speed = value;
        break;
    default:
        return 0u;
    }

    g_flash_modified = 1u;
    printf("FLASH: cached id=0x%02X val=%.4f\r\n",
           (unsigned int)id, (double)value);
    return 1u;
}

uint8_t flash_store_is_modified(void)
{
    return g_flash_modified;
}

void flash_store_save(void)
{
    HAL_StatusTypeDef status;
    uint32_t addr;
    uint32_t *src;
    uint32_t i;
    uint32_t word_count;
    flash_params_t params;

    /* Apply cached values to runtime variables via setters */
    motor1_set_kp(g_flash_params.motor1_kp);
    motor1_set_ki(g_flash_params.motor1_ki);
    motor2_set_kp(g_flash_params.motor2_kp);
    motor2_set_kd(g_flash_params.motor2_kd);

    /* Build struct for Flash write */
    params.magic           = FLASH_PARAMS_MAGIC;
    params.motor1_kp       = g_flash_params.motor1_kp;
    params.motor1_ki       = g_flash_params.motor1_ki;
    params.motor1_max_speed = g_flash_params.motor1_max_speed;
    params.motor2_kp       = g_flash_params.motor2_kp;
    params.motor2_kd       = g_flash_params.motor2_kd;
    params.motor2_max_speed = g_flash_params.motor2_max_speed;
    params.checksum   = 0u;
    params.checksum   = flash_checksum(&params);

    printf("FLASH: saving Kp1=%.4f Ki1=%.4f Spd1=%.1f Kp2=%.4f Kd2=%.4f Spd2=%.1f\r\n",
           (double)params.motor1_kp,
           (double)params.motor1_ki,
           (double)params.motor1_max_speed,
           (double)params.motor2_kp,
           (double)params.motor2_kd,
           (double)params.motor2_max_speed);

    /* Unlock Flash */
    HAL_FLASH_Unlock();

    /* Erase sector */
    {
        FLASH_EraseInitTypeDef erase = {0};
        uint32_t sector_error = 0u;

        erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
        erase.Sector       = FLASH_PARAMS_SECTOR;
        erase.NbSectors    = 1u;
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

        status = HAL_FLASHEx_Erase(&erase, &sector_error);
        if (status != HAL_OK)
        {
            printf("FLASH: erase failed! status=%d err_sector=%lu\r\n",
                   (int)status, (unsigned long)sector_error);
            HAL_FLASH_Lock();
            return;
        }
    }

    /* Program word by word */
    addr       = FLASH_PARAMS_ADDR;
    src        = (uint32_t *)&params;
    word_count = (uint32_t)((sizeof(flash_params_t) + 3u) / 4u);

    for (i = 0u; i < word_count; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]);
        if (status != HAL_OK)
        {
            printf("FLASH: program failed at word %lu status=%d\r\n",
                   (unsigned long)i, (int)status);
            HAL_FLASH_Lock();
            return;
        }
        addr += 4u;
    }

    /* Lock Flash */
    HAL_FLASH_Lock();

    /* Clear modified flag */
    g_flash_modified = 0u;

    printf("FLASH: save OK\r\n");
}
