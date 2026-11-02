#include "rs00_mit.h"
#include <stdio.h>
#include <string.h>

#include "rs00_motor.h"

#define test

/* CAN ������Ϣ�ṹ */
static CAN_TxHeaderTypeDef txHeader;
static uint8_t txData[8];
static uint32_t txMailbox;

/* MIT Э���ʼ�� */
void MIT_Init(MIT_StateTypeDef *mit_state)
{
    if (mit_state == NULL) return;
    
    mit_state->motor_id = Motor_id;        // Ĭ�ϵ�� ID
    mit_state->master_id = Master_id;       // Ĭ������ ID
	
    mit_state->mode = MIT_MODE_MIT;    // Ĭ�� MIT ģʽ
    mit_state->fault_status = 0;       // �޹���
    mit_state->position = 0.0f;
    mit_state->velocity = 0.0f;
    mit_state->torque = 0.0f;
    mit_state->temperature = 0.0f;
}

/* ===================== MIT ָ��ͺ��� ===================== */

/* ���ʹ�����У�ָ��1�� */
void MIT_SendEnable(uint8_t motor_id)
{
    txHeader.StdId = motor_id;         // ��׼֡����� ID
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF FF FC
    for (int i = 0; i < 7; i++) {
        txData[i] = 0xFF;
    }
    txData[7] = 0xFC;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Send ENABLE command to Motor[0x%02X]\r\n", motor_id);
}

/* ���ֹͣ���У�ָ��2�� */
void MIT_SendStop(uint8_t motor_id)
{
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF FF FD
    for (int i = 0; i < 7; i++) {
        txData[i] = 0xFF;
    }
    txData[7] = 0xFD;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Send STOP command to Motor[0x%02X]\r\n", motor_id);
}

/* MIT ��̬����ָ�ָ��3�� */
void MIT_SendMITParams(uint8_t motor_id, MIT_ControlParamsTypeDef *params)
{
    if (params == NULL) return;
    
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // Ŀ��Ƕ� [0~65535]��Ӧ(-12.57rad~12.57rad)
    uint16_t position_i = (uint16_t)((params->position_ref + 12.57f) / 25.14f * 65535.0f);
    txData[0] = (position_i >> 8) & 0xFF;
    txData[1] = position_i & 0xFF;
    
    // Ŀ���ٶ� [0~4096]��Ӧ(-33rad/s~33rad/s)
    uint16_t velocity_i = (uint16_t)((params->velocity_ref + 33.0f) / 66.0f * 4096.0f);
    txData[2] = (velocity_i >> 4) & 0xFF;  // ��8λ
    
    // Kp [0~4096]��Ӧ(0-500)
    uint16_t kp_i = (uint16_t)(params->kp * 4096.0f / 500.0f);
    txData[3] = ((velocity_i & 0x0F) << 4) | ((kp_i >> 8) & 0x0F);
    
    // Kd [0~4096]��Ӧ(0-5)
    uint16_t kd_i = (uint16_t)(params->kd * 4096.0f / 5.0f);
    txData[4] = kp_i & 0xFF;
    txData[5] = (kd_i >> 4) & 0xFF;
    
    // Ŀ������ [0~4096]��Ӧ(-14N.m~14N.m)
    uint16_t torque_i = (uint16_t)((params->torque_ref + 14.0f) / 28.0f * 4096.0f);
    txData[6] = ((kd_i & 0x0F) << 4) | ((torque_i >> 8) & 0x0F);
    txData[7] = torque_i & 0xFF;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Send MIT Params to Motor[0x%02X]\r\n", motor_id);
}

/* ������㣨ָ��4�� */
void MIT_SendSetZero(uint8_t motor_id)
{
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF FF FE
    for (int i = 0; i < 7; i++) {
        txData[i] = 0xFF;
    }
    txData[7] = 0xFE;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Send SET ZERO command to Motor[0x%02X]\r\n", motor_id);
}

/* ������󼰶�ȡ�쳣״̬��ָ��5�� */
void MIT_SendClearError(uint8_t motor_id)
{
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF FF FB
    for (int i = 0; i < 7; i++) {
        txData[i] = 0xFF;
    }
    txData[7] = 0xFB;
    
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif	
	
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Send CLEAR ERROR command to Motor[0x%02X]\r\n", motor_id);
}

/* ��������ģʽ��ָ��6�� */
void MIT_SendSetMode(uint8_t motor_id, uint8_t mode)
{
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF F_CMD FC
    for (int i = 0; i < 6; i++) {
        txData[i] = 0xFF;
    }
    txData[6] = mode;  // F_CMD �ֽ�Ϊ����ģʽ
    txData[7] = 0xFC;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif	
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Set mode to 0x%02X for Motor[0x%02X]\r\n", mode, motor_id);
}

/* �޸ĵ�� CAN_ID��ָ��7�� */
void MIT_SendSetMotorID(uint8_t old_id, uint8_t new_id)
{
    txHeader.StdId = old_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF 01 FA
    for (int i = 0; i < 6; i++) {
        txData[i] = 0xFF;
    }
    txData[6] = new_id;  // �µĵ�� ID
    txData[7] = 0xFA;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif	
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Change Motor ID from 0x%02X to 0x%02X\r\n", old_id, new_id);
}

/* �޸ĵ��Э�飨ָ��8�� */
void MIT_SendSetProtocol(uint8_t motor_id, uint8_t protocol)
{
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF 00 FD
    for (int i = 0; i < 6; i++) {
        txData[i] = 0xFF;
    }
    txData[6] = protocol;  // Э�����ͣ�0=˽�У�1=Canopen��2=MIT
    txData[7] = 0xFD;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif	
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Set protocol to 0x%02X for Motor[0x%02X]\r\n", protocol, motor_id);
}

/* �޸����� canid��ָ��9�� */
void MIT_SendSetMasterID(uint8_t motor_id, uint8_t master_id)
{
    txHeader.StdId = motor_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // ������FF FF FF FF FF FF FD 01
    for (int i = 0; i < 6; i++) {
        txData[i] = 0xFF;
    }
    txData[6] = master_id;  // �µ����� ID
    txData[7] = 0x01;
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif	
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Set Master ID to 0x%02X for Motor[0x%02X]\r\n", master_id, motor_id);
}

/* λ��ģʽ����ָ�ָ��10�� */
void MIT_SendPositionCommand(uint8_t motor_id, float position, float velocity)
{
    txHeader.StdId = (1 << 8) | motor_id;  // ��3λΪ1����8λΪ���ID
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // Byte0~3: Ŀ��λ�ã���λrad��32λ������float
    MIT_FloatToBytes(position, &txData[0]);
    
    // Byte4~7: Ŀ���ٶȣ���λrad/s��32λ������float
    MIT_FloatToBytes(velocity, &txData[4]);
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Position command - Pos=%.3f rad, Vel=%.3f rad/s\r\n", position, velocity);
}

/* �ٶ�ģʽ����ָ�ָ��11�� */
void MIT_SendVelocityCommand(uint8_t motor_id, float velocity, float current_limit)
{
    txHeader.StdId = (2 << 8) | motor_id;  // ��3λΪ2����8λΪ���ID
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    // Byte0~3: Ŀ���ٶȣ���λrad/s��32λ������float
    MIT_FloatToBytes(velocity, &txData[0]);
    
    // Byte4~7: �ٶ�λ��ģʽ�������ƣ���λA��32λ������float
    MIT_FloatToBytes(current_limit, &txData[4]);
	
	#ifdef test
	for(int i=0; i<8; i++) {
        printf("%02X ", txData[i]);
    }
	printf("\r\n");
	#endif
    
    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);
    printf("MIT: Velocity command - Vel=%.3f rad/s, Current limit=%.3f A\r\n", velocity, current_limit);
}

/* ������ת�ֽ����飨IEEE754 С���� */
void MIT_FloatToBytes(float value, uint8_t *bytes)
{
    uint32_t temp;
    memcpy(&temp, &value, sizeof(float));
    for (int i = 0; i < 4; i++) {
        bytes[i] = (temp >> (i * 8)) & 0xFF;
    }
}

/* �ֽ�����ת������ */
float MIT_BytesToFloat(uint8_t *bytes)
{
    uint32_t temp = 0;
    for (int i = 0; i < 4; i++) {
        temp |= (bytes[i] << (i * 8));
    }
    float result;
    memcpy(&result, &temp, sizeof(float));
    return result;
}

/* ===================== MIT ����ת������ ===================== */

float MIT_GetPosition(uint8_t *data)
{
    uint16_t raw = ((uint16_t)data[1] << 8) | data[2];
    return ((float)raw / 65535.0f) * 25.14f - 12.57f;
}

float MIT_GetVelocity(uint8_t *data)
{
    // �ٶ��� Byte3 ��ȫ 8 λ + Byte4 �ĸ� 4 λƴ�ӳ� 12 λ
    // Byte3 Ϊ��8λ��Byte4[7~4]Ϊ��4λ
    uint16_t raw = ((data[3] << 4) | ((data[4] & 0xF0) >> 4));
	
    return ((float)raw / 4096.0f) * 66.0f - 33.0f;
}

float MIT_GetTorque(uint8_t *data)
{
    // Ť���� Byte4 �ĵ� 4 λ + Byte5 ȫ 8 λ��� 12 λ
    // Byte4[3~0]Ϊ��4λ��Byte5Ϊ��8λ
    uint16_t raw = ((data[4] & 0x0F) << 8) | data[5];
    return ((float)raw / 4096.0f) * 28.0f - 14.0f;
}

float MIT_GetTemperature(uint8_t *data)
{
    uint16_t raw = ((uint16_t)data[6] << 8) | data[7];
    return ((float)raw) / 10.0f;  // ��λ��0.1��
}

/* ---------------- ���������� ---------------- */
void MIT_ParseResponse(uint32_t std_id, uint8_t *data, uint8_t dlc, MIT_StateTypeDef *state)
{
    if (!state || dlc < 8) return;

    printf("\r\n=== [MIT Response Frame] ===\r\n");
    printf("  StdID : 0x%03X\r\n", std_id);
    printf("  Data  : ");
    for (int i = 0; i < dlc; i++) printf("%02X ", data[i]);
    printf("\r\n");

    uint8_t type = (std_id >> 8) & 0x0F;
//    uint8_t id   = std_id & 0xFF;

    /* ---- Ӧ��ָ�� 1: ���״̬���� ---- */
    if (type == 0x00)
    {
        printf("  Response Type : Motor State Feedback\r\n");

        state->motor_id   = data[0];
        state->position   = MIT_GetPosition(data);
        state->velocity   = MIT_GetVelocity(data);
        state->torque     = MIT_GetTorque(data);
        state->temperature= MIT_GetTemperature(data);

        printf("    Motor ID : 0x%02X\r\n", state->motor_id);
        printf("    Position : %7.3f rad\r\n", state->position);
        printf("    Velocity : %7.3f rad/s\r\n", state->velocity);
        printf("    Torque   : %7.3f N��m\r\n", state->torque);
        printf("    Temp.    : %5.1f ��C\r\n", state->temperature);
    }
    else
    {
        printf("  Response Type : Unknown (type=0x%02X)\r\n", type);
    }
}

