/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "delay.h"
#include "AD7606.h"

#include "controller.h"

#include "rs00_motor.h"
#include "rs00_mit.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void set_motor_pwm(float pwm1,float pwm2);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PWM1_Pin GPIO_PIN_5
#define PWM1_GPIO_Port GPIOE
#define PWM2_Pin GPIO_PIN_6
#define PWM2_GPIO_Port GPIOE
#define LED_RUN_Pin GPIO_PIN_0
#define LED_RUN_GPIO_Port GPIOC
#define UART2_TX_Pin GPIO_PIN_5
#define UART2_TX_GPIO_Port GPIOD
#define UART2_RX_Pin GPIO_PIN_6
#define UART2_RX_GPIO_Port GPIOD
#define FMC_D4_Pin GPIO_PIN_7
#define FMC_D4_GPIO_Port GPIOE
#define FMC_D5_Pin GPIO_PIN_8
#define FMC_D5_GPIO_Port GPIOE
#define FMC_D6_Pin GPIO_PIN_9
#define FMC_D6_GPIO_Port GPIOE
#define FMC_D7_Pin GPIO_PIN_10
#define FMC_D7_GPIO_Port GPIOE
#define FMC_D8_Pin GPIO_PIN_11
#define FMC_D8_GPIO_Port GPIOE
#define FMC_D9_Pin GPIO_PIN_12
#define FMC_D9_GPIO_Port GPIOE
#define FMC_D10_Pin GPIO_PIN_13
#define FMC_D10_GPIO_Port GPIOE
#define FMC_D11_Pin GPIO_PIN_14
#define FMC_D11_GPIO_Port GPIOE
#define FMC_D12_Pin GPIO_PIN_15
#define FMC_D12_GPIO_Port GPIOE
#define FMC_D13_Pin GPIO_PIN_8
#define FMC_D13_GPIO_Port GPIOD
#define FMC_D14_Pin GPIO_PIN_9
#define FMC_D14_GPIO_Port GPIOD
#define FMC_D15_Pin GPIO_PIN_10
#define FMC_D15_GPIO_Port GPIOD
#define AD7606_OS2_Pin GPIO_PIN_11
#define AD7606_OS2_GPIO_Port GPIOD
#define AD7606_OS1_Pin GPIO_PIN_12
#define AD7606_OS1_GPIO_Port GPIOD
#define AD7606_OS0_Pin GPIO_PIN_13
#define AD7606_OS0_GPIO_Port GPIOD
#define FMC_D0_Pin GPIO_PIN_14
#define FMC_D0_GPIO_Port GPIOD
#define FMC_D1_Pin GPIO_PIN_15
#define FMC_D1_GPIO_Port GPIOD
#define AD7606_RANGE_Pin GPIO_PIN_8
#define AD7606_RANGE_GPIO_Port GPIOC
#define AD7606_CONVST_Pin GPIO_PIN_9
#define AD7606_CONVST_GPIO_Port GPIOC
#define AD7606_RD_Pin GPIO_PIN_8
#define AD7606_RD_GPIO_Port GPIOA
#define UART1_TX_Pin GPIO_PIN_9
#define UART1_TX_GPIO_Port GPIOA
#define UART1_RX_Pin GPIO_PIN_10
#define UART1_RX_GPIO_Port GPIOA
#define CAN_Motor_Rx_Pin GPIO_PIN_11
#define CAN_Motor_Rx_GPIO_Port GPIOA
#define CAN_Motor_Tx_Pin GPIO_PIN_12
#define CAN_Motor_Tx_GPIO_Port GPIOA
#define AD7606_RESET_Pin GPIO_PIN_15
#define AD7606_RESET_GPIO_Port GPIOA
#define FMC_D2_Pin GPIO_PIN_0
#define FMC_D2_GPIO_Port GPIOD
#define FMC_D3_Pin GPIO_PIN_1
#define FMC_D3_GPIO_Port GPIOD
#define AD7606_CS_Pin GPIO_PIN_3
#define AD7606_CS_GPIO_Port GPIOD
#define AD7606_BUSY_Pin GPIO_PIN_4
#define AD7606_BUSY_GPIO_Port GPIOD
#define AD7606_FIRSTDATA_Pin GPIO_PIN_7
#define AD7606_FIRSTDATA_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
