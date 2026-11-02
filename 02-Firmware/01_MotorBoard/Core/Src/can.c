/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN 0 */

#include "rs00_motor.h"
#include <stdio.h>
#include <string.h>
#include "rs00_mit.h"

static CAN_Frame_t can_rx_buffer[CAN_RX_BUFFER_SIZE];
static volatile uint8_t can_rx_head = 0;
static volatile uint8_t can_rx_tail = 0;

extern MIT_StateTypeDef mit_state;

/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_4TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */
  
  CAN_FilterTypeDef sFilterConfig;

	/* 配置滤波器：接收所有帧 */
	sFilterConfig.FilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	/* 启动 CAN 硬件 */
	if (HAL_CAN_Start(&hcan1) != HAL_OK)
	{
		Error_Handler();
	}

	/* 开启接收中断通知 */
	if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
	{
		Error_Handler();
	}

	printf("CAN1 initialized and started successfully.\r\n");
  

  /* USER CODE END CAN1_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = CAN_Motor_Rx_Pin|CAN_Motor_Tx_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, CAN_Motor_Rx_Pin|CAN_Motor_Tx_Pin);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/* 检查缓存是否已满 */
static inline uint8_t CAN_BufferFull(void)
{
    return ((can_rx_head + 1) % CAN_RX_BUFFER_SIZE) == can_rx_tail;
}

/* 将接收到的数据写入环形缓存 */
static void CAN_PushToBuffer(uint32_t id, uint8_t dlc, uint8_t *data,uint8_t is_extended)
{
    if (CAN_BufferFull())
    {
       
        can_rx_tail = (can_rx_tail + 1) % CAN_RX_BUFFER_SIZE;
    }

    can_rx_buffer[can_rx_head].id = id;
    can_rx_buffer[can_rx_head].dlc = dlc;
	can_rx_buffer[can_rx_head].is_extended = is_extended;
    memcpy(can_rx_buffer[can_rx_head].data, data, dlc);
    can_rx_head = (can_rx_head + 1) % CAN_RX_BUFFER_SIZE;
}

uint8_t CAN_PopFrame(CAN_Frame_t *frame)
{
    if (can_rx_head == can_rx_tail)
        return 0; 

    *frame = can_rx_buffer[can_rx_tail];
    can_rx_tail = (can_rx_tail + 1) % CAN_RX_BUFFER_SIZE;
    return 1;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
    {
		uint8_t is_extended = (rxHeader.IDE == CAN_ID_EXT);
        uint32_t id = is_extended ? rxHeader.ExtId : rxHeader.StdId;
        
        /* 将帧存入缓冲区 */
        CAN_PushToBuffer(id, rxHeader.DLC, rxData, is_extended);
//         if (is_extended)
//        {
//            uint8_t comm_type = (id >> 24) & 0x1F;
//            if (comm_type == 0x02)   // 电机反馈帧
//            {
//                // 解析角度、速度、力矩、温度，更新 motor_feedback
//                RS00_GetFeedbackFrame(rxData, &motor_feedback);
//                // 可选：打印调试信息
//                // printf("Position: %.3f rad\n", motor_feedback.position);
//            }
//        }
		
//        /* 快速识别关键帧（可选） */
//        if (is_extended && id == 0x00007FFE)
//        {
//            printf("Device Power-On Broadcast Detected! UID = ");
//            for (int i = 0; i < 8; i++) printf("%02X", rxData[i]);
//            printf("\r\n");
//        }

    }
}



void CAN_ProcessReceivedFrames(void)
{
	
	CAN_Frame_t frame;

    while (CAN_PopFrame(&frame))
    {
        
            /* 扩展帧：私有协议 */
            RS00_ParseCANFrame(frame.id, frame.data, frame.dlc);
        
    }
	
//    CAN_Frame_t frame;

//    while (CAN_PopFrame(&frame))
//    {
//        uint32_t extid = frame.id;
//		
//        uint8_t comm_type = (extid >> 24) & 0x1F;
//		
//        uint8_t motor_id  = (extid >> 8) & 0xFF;   // ���������ĵ��ID
//		
////        uint8_t master_id = (extid >> 0) & 0xFF;   // ���ǵ�����ID��Ŀ��ID��

////        printf("\r\n=== [CAN RX FRAME] ===\r\n");
////        printf("ExtID: 0x%08X | Type: 0x%02X | Master: 0x%02X | Motor: 0x%02X | DLC: %d\r\n",
////               extid, comm_type, master_id, motor_id, frame.dlc);
////        printf("Data: ");
////        for (int i = 0; i < frame.dlc; i++) printf("%02X ", frame.data[i]);
////        printf("\r\n");

//        /* ����ͨ�����ͷ������ */
//        switch (comm_type)
//        {
//            case 0x00:   // �豸��Ϣ�ظ�
//                printf("?? Device Info Reply (MotorID=0x%02X)\r\n", motor_id);
//                printf("  Raw UID: ");
//                for (int i = 0; i < frame.dlc; i++) printf("%02X", frame.data[i]);
//                
//				printf("\r\n");
//                break;
//            case 0x02:   // ���״̬����
//                RS00_GetFeedbackFrame(frame.data, &motor_feedback);
//                printf("??  Motor[0x%02X] State:\r\n", motor_id);
//                printf("  Pos = %.3f rad\r\n", motor_feedback.position);
//                printf("  Vel = %.3f rad/s\r\n", motor_feedback.speed);
//                printf("  Tor = %.3f N��m\r\n", motor_feedback.torque);
//                printf("  Tmp = %.1f ��C\r\n", motor_feedback.temperature);
//                break;
//            case 0x03:
//                printf("? Motor[0x%02X] Enable Ack\r\n", motor_id);
//                break;
//            case 0x04:
//                printf("?? Motor[0x%02X] Stop Ack\r\n", motor_id);
//                break;

//            default:
//                printf("?? Unknown Frame Type: 0x%02X\r\n", comm_type);
//                break;
//        }
//    }
}



/* USER CODE END 1 */
