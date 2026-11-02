/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "controller.h"
#include "rs00_motor.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */

#define USART_REC_LEN   200
#define RXBUFFERSIZE    1
#define UART3_DMA_RX_BUF_SIZE  256

extern uint8_t uart_dma_busy;

/* UART3 DMA RX/TX state */
extern volatile uint8_t  g_usart3_dma_rx_ready;
extern volatile uint16_t g_usart3_dma_rx_len;
extern uint8_t  g_usart3_dma_rx_buf[UART3_DMA_RX_BUF_SIZE];
extern volatile uint8_t  g_usart3_dma_tx_busy;
extern volatile uint32_t g_usart3_dma_rx_count;

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */

extern uint8_t g_usart2_rx_sta;                 
extern uint8_t  g_usart2_rx_buffer[RXBUFFERSIZE];           

void uart2_SendDatas(uint8_t *pData, uint16_t Size);

void uart_send_10float(float *data);
void USART3_SendString(const char *msg);
uint8_t UART3_SendDMA(uint8_t *pData, uint16_t Size);
void UART3_RxPoll(void);
void USART2_DebugPoll(void);
void USART2_DebugSendString(const char *msg);
void USART2_DR304PrintTask(void);
void USART2_DR304GetLatestValues(float values[6]);
void USART2_DR304SetZero(void);

/* Send a small test frame on USART2 for quick verification */
void USART2_SendTestFrame(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

