#ifndef RS00_MIT_H
#define RS00_MIT_H

#include "stm32f4xx_hal.h"
#include "can.h"
#include <stdint.h>

/* MIT 协议相关定义 */
#define MIT_PROTOCOL_ID        0x02    // MIT 协议标识

/* MIT 指令类型定义 */
#define MIT_CMD_ENABLE         0x01    // 电机使能运行
#define MIT_CMD_STOP           0x02    // 电机停止运行
#define MIT_CMD_MIT_PARAMS     0x03    // MIT 动态参数
#define MIT_CMD_SET_ZERO       0x04    // 设置零点
#define MIT_CMD_CLEAR_ERROR    0x05    // 清除错误及读取异常状态
#define MIT_CMD_SET_MODE       0x06    // 设置运行模式
#define MIT_CMD_SET_MOTOR_ID   0x07    // 修改电机 CAN_ID
#define MIT_CMD_SET_PROTOCOL   0x08    // 修改电机协议
#define MIT_CMD_SET_MASTER_ID  0x09    // 修改主机 canid
#define MIT_CMD_POSITION       0x10    // 位置模式控制指令
#define MIT_CMD_VELOCITY       0x11    // 速度模式控制指令

/* MIT 运行模式 */
#define MIT_MODE_MIT           0x00    // MIT 模式（默认）
#define MIT_MODE_POSITION      0x01    // 位置模式
#define MIT_MODE_VELOCITY      0x02    // 速度模式

/* MIT 协议结构体定义 */
typedef struct {
    uint8_t motor_id;           // 电机 CAN ID
    uint8_t master_id;          // 主机 CAN ID
    uint8_t mode;               // 当前运行模式
    uint32_t fault_status;      // 故障状态
    float position;             // 当前位置
    float velocity;             // 当前速度
    float torque;               // 当前扭矩
    float temperature;          // 当前温度
} MIT_StateTypeDef;

/* MIT 控制参数结构体 */
typedef struct {
    float position_ref;         // 目标位置 (rad)
    float velocity_ref;         // 目标速度 (rad/s)
    float torque_ref;          // 目标扭矩 (N.m)
    float kp;                  // 位置增益
    float kd;                  // 速度增益
} MIT_ControlParamsTypeDef;

/* MIT 函数声明 */

/* 初始化 MIT 协议 */
void MIT_Init(MIT_StateTypeDef *mit_state);

/* 发送 MIT 指令 */
void MIT_SendEnable(uint8_t motor_id);
void MIT_SendStop(uint8_t motor_id);
void MIT_SendMITParams(uint8_t motor_id, MIT_ControlParamsTypeDef *params);
void MIT_SendSetZero(uint8_t motor_id);
void MIT_SendClearError(uint8_t motor_id);
void MIT_SendSetMode(uint8_t motor_id, uint8_t mode);
void MIT_SendSetMotorID(uint8_t old_id, uint8_t new_id);
void MIT_SendSetProtocol(uint8_t motor_id, uint8_t protocol);
void MIT_SendSetMasterID(uint8_t motor_id, uint8_t master_id);
void MIT_SendPositionCommand(uint8_t motor_id, float position, float velocity);
void MIT_SendVelocityCommand(uint8_t motor_id, float velocity, float current_limit);

/* 解析 MIT 反馈帧 */
void MIT_ParseResponse(uint32_t std_id, uint8_t *data, uint8_t dlc, MIT_StateTypeDef *state);

/* 获取 MIT 反馈数据 */
float MIT_GetPosition(uint8_t *data);
float MIT_GetVelocity(uint8_t *data);
float MIT_GetTorque(uint8_t *data);
float MIT_GetTemperature(uint8_t *data);

/* 将浮点数转换为 IEEE 754 字节 */
void MIT_FloatToBytes(float value, uint8_t *bytes);

#endif /* RS00_MIT_H */
