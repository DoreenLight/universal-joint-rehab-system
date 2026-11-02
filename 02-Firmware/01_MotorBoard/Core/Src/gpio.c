/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED_RUN_Pin|AD7606_RANGE_Pin|AD7606_CONVST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, AD7606_OS2_Pin|AD7606_OS1_Pin|AD7606_OS0_Pin|AD7606_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, AD7606_RD_Pin|AD7606_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_RUN_Pin */
  GPIO_InitStruct.Pin = LED_RUN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LED_RUN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : FMC_D4_Pin FMC_D5_Pin FMC_D6_Pin FMC_D7_Pin
                           FMC_D8_Pin FMC_D9_Pin FMC_D10_Pin FMC_D11_Pin
                           FMC_D12_Pin */
  GPIO_InitStruct.Pin = FMC_D4_Pin|FMC_D5_Pin|FMC_D6_Pin|FMC_D7_Pin
                          |FMC_D8_Pin|FMC_D9_Pin|FMC_D10_Pin|FMC_D11_Pin
                          |FMC_D12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : FMC_D13_Pin FMC_D14_Pin FMC_D15_Pin FMC_D0_Pin
                           FMC_D1_Pin FMC_D2_Pin FMC_D3_Pin AD7606_BUSY_Pin
                           AD7606_FIRSTDATA_Pin */
  GPIO_InitStruct.Pin = FMC_D13_Pin|FMC_D14_Pin|FMC_D15_Pin|FMC_D0_Pin
                          |FMC_D1_Pin|FMC_D2_Pin|FMC_D3_Pin|AD7606_BUSY_Pin
                          |AD7606_FIRSTDATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : AD7606_OS2_Pin AD7606_OS1_Pin AD7606_OS0_Pin AD7606_CS_Pin */
  GPIO_InitStruct.Pin = AD7606_OS2_Pin|AD7606_OS1_Pin|AD7606_OS0_Pin|AD7606_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : AD7606_RANGE_Pin AD7606_CONVST_Pin */
  GPIO_InitStruct.Pin = AD7606_RANGE_Pin|AD7606_CONVST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : AD7606_RD_Pin AD7606_RESET_Pin */
  GPIO_InitStruct.Pin = AD7606_RD_Pin|AD7606_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */
void My_Own_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIOC 时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PC3 配置为上升沿中断 */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* EXTI3 中断配置 */
    HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/* USER CODE END 2 */
