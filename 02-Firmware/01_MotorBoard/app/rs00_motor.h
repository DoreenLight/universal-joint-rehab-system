#ifndef RS00_MOTOR_H
#define RS00_MOTOR_H

#include "stm32f4xx_hal.h"
#include "can.h"
#include <stdint.h>

#define RS00_P_MIN   -12.57f
#define RS00_P_MAX    12.57f
#define RS00_V_MIN   -33.0f
#define RS00_V_MAX    33.0f
#define RS00_T_MIN   -14.0f
#define RS00_T_MAX    14.0f
#define RS00_KP_MIN    0.0f
#define RS00_KP_MAX  500.0f
#define RS00_KD_MIN    0.0f
#define RS00_KD_MAX    5.0f

#define Master_id	0xFD
#define Motor_id	0x05

#define HOMING_IDLE    0
#define HOMING_RUNNING 1
#define HOMING_SUCCESS 2
#define HOMING_ERROR   -1
//#define Motor_id	0x6F

// 通信类型定义
#define CMD_TYPE_ENABLE      0x03  // 使能运行
#define CMD_TYPE_STOP        0x04  // 停止运行
#define CMD_TYPE_CONTROL     0x01  // 运控模式控制
#define CMD_TYPE_READ_STATE  0x02  // 读取状态
#define CMD_TYPE_SET_ZERO    0x06  // 设置零位
#define CMD_TYPE_READ_ID     0x00  // 读取设备ID
#define CMD_TYPE_READ_PARAM  0x11  // 单个参数读取（通信类型17）
#define CMD_TYPE_WRITE_PARAM  0x12  // 单个参数写入（通信类型18）

// 运行模式定义（说明书第36页）
#define RUN_MODE_OPERATION    0    // 运控模式
#define RUN_MODE_PP           1    // 位置模式（PP）
#define RUN_MODE_VELOCITY     2    // 速度模式
#define RUN_MODE_CURRENT      3    // 电流模式
#define RUN_MODE_CSP          5    // 位置模式（CSP）

// 参数索引定义（根据说明书第36-37页）
#define PARAM_INDEX_RUN_MODE         0x7005  // 运行模式
#define PARAM_INDEX_VEL_MAX          0x7024  // PP模式最大速度
#define PARAM_INDEX_ACC_SET          0x7025  // PP模式加速度
#define PARAM_INDEX_LOC_REF          0x7016  // 位置指令（CSP/PP）
#define PARAM_INDEX_SPD_REF          0x700A  // 速度指令
#define PARAM_INDEX_ACC_RAD          0x7022  // 速度模式加速度
#define PARAM_INDEX_LIMIT_SPD        0x7017  // CSP模式速度限制
#define PARAM_INDEX_LIMIT_CUR        0x7018  // 速度位置模式电流限制（需要添加这个）
#define PARAM_INDEX_LIMIT_TORQUE     0x700B  // 转矩限制（可选添加）

// PID和控制参数索引（根据您提供的图片）
#define PARAM_INDEX_LOC_KP           0x701E  // 位置环KP
#define PARAM_INDEX_SPD_KP           0x701F  // 速度环KP
#define PARAM_INDEX_SPD_KI           0x7020  // 速度环KI
#define PARAM_INDEX_SPD_FILTER_GAIN  0x7021  // 速度滤波值

// 其他可能需要的控制参数（根据说明书第36-37页）
#define PARAM_INDEX_CUR_KP           0x7010  // 电流环KP
#define PARAM_INDEX_CUR_KI           0x7011  // 电流环KI
#define PARAM_INDEX_CUR_FILTER_GAIN  0x7014  // 电流滤波参数
#define PARAM_INDEX_IQ_REF           0x7006  // 电流模式Iq指令

/* 协议类型定义 */
#define PROTOCOL_PRIVATE  0  // 私有协议
#define PROTOCOL_CANOPEN  1  // Canopen协议
#define PROTOCOL_MIT      2  // MIT协议

/* 波特率定义 */
#define BAUD_RATE_1M    1    // 1 Mbps
#define BAUD_RATE_500K  2    // 500 Kbps
#define BAUD_RATE_250K  3    // 250 Kbps
#define BAUD_RATE_125K  4    // 125 Kbps


typedef struct {
    uint8_t id;
    float position;
    float speed;
    float torque;
    float kp;
    float kd;
} RS00_ControlTypeDef;

typedef struct {
    float position;
    float speed;
    float torque;
    float temperature;
    uint32_t fault;
} RS00_FeedbackTypeDef;

extern RS00_FeedbackTypeDef motor_feedback;

float CalculateShortestTarget(float current);
void RS00_GetFeedbackFrame(uint8_t *rxData, RS00_FeedbackTypeDef *fb);
void RS00_StartHoming(uint8_t motor_id,uint8_t master_id);
/* 初始化电机CAN */
void RS00_CAN_Init(void);
extern int8_t g_homing_stage;
/* 发送电机使能命令 */
void RS00_Enable(uint8_t motor_id, uint16_t master_id);

/* 发送停止命令 */
void RS00_Stop(uint8_t motor_id, uint8_t master_id);
void RS00_StopAndClearError(uint8_t motor_id, uint8_t master_id);
/* 发送运控模式指令 */
void RS00_Control(uint8_t motor_id, uint8_t master_id, float position, float speed, float torque, float kp, float kd);

/* 发送位置模式(CSP/PP)指令 */
void RS00_Position(uint8_t motor_id, float position, float speed, float acceleration);

/* 读取电机反馈 */
void RS00_GetFeedback(uint8_t motor_id, RS00_FeedbackTypeDef *feedback);
uint8_t RS00_Calculate_Homing(float *out_pos, float *out_kp, float *out_kd);
/* 浮点转换为整数协议函数 */
int RS00_FloatToUInt(float x, float x_min, float x_max, int bits);

/* 发送通信类型0：读取设备ID */
void RS00_ReadDeviceID(uint8_t motor_id, uint8_t master_id);

/* 主控发送通信类型 0x02，请求电机返回反馈帧 */
void RS00_ReadMotorState(uint8_t motor_id, uint8_t master_id);

void RS00_SetZeroPosition(uint8_t motor_id, uint8_t master_id);

// 新增函数声明
void RS00_WriteParamUint8(uint8_t motor_id, uint8_t master_id, uint16_t index, uint8_t value);
void RS00_WriteParamFloat(uint8_t motor_id, uint8_t master_id, uint16_t index, float value);
void RS00_SetRunMode(uint8_t motor_id, uint8_t master_id, uint8_t run_mode);
void RS00_SetPositionModeParams(uint8_t motor_id, uint8_t master_id, float max_velocity, float acceleration);
void RS00_SetVelocityModeParams(uint8_t motor_id, uint8_t master_id, float max_current, float acceleration);
void RS00_SendPositionCommand(uint8_t motor_id, uint8_t master_id, float position);
void RS00_SendVelocityCommand(uint8_t motor_id, uint8_t master_id, float velocity);

// 新增控制参数设置函数声明
void RS00_SetPositionControlParams(uint8_t motor_id, uint8_t master_id, float kp);
void RS00_SetSpeedControlParams(uint8_t motor_id, uint8_t master_id, float kp, float ki, float filter_gain);
void RS00_SetCurrentControlParams(uint8_t motor_id, uint8_t master_id, float kp, float ki, float filter_gain);

void RS00_ReadParam(uint8_t motor_id, uint8_t master_id, uint16_t index);
void RS00_SetSpeedMode_Control(uint8_t motor_id, uint8_t master_id, float target_speed);
void RS00_ConfigSpeedMode(uint8_t motor_id, uint8_t master_id);
void RS00_WriteSpeed(uint8_t motor_id, uint8_t master_id, float target_speed);
// 在现有函数声明后面添加：

/* 参数读取/写入应答解析函数 */
void RS00_ParseReadParamReply(uint8_t motor_id, uint8_t *data, uint8_t dlc);
void RS00_ParseWriteParamAck(uint8_t motor_id, uint8_t *data, uint8_t dlc);
void RS00_ParseFaultFrame(uint8_t motor_id, uint8_t *data, uint8_t dlc);
void RS00_ParseActiveReport(uint8_t motor_id, uint8_t *data, uint8_t dlc);

/* 通用CAN帧解析入口函数 */
void RS00_ParseCANFrame(uint32_t extid, uint8_t *data, uint8_t dlc);

/* 设置电机协议类型 */
void RS00_SetProtocolType(uint8_t motor_id, uint8_t master_id, uint8_t protocol);
void RS00_SetProtocolTypeByParam(uint8_t motor_id, uint8_t master_id, uint8_t protocol);

/* 保存参数 */
void RS00_SaveParameters(uint8_t motor_id, uint8_t master_id);

/* 设置波特率 */
void RS00_SetBaudRate(uint8_t motor_id, uint8_t master_id, uint8_t baud_rate);

/* 解析协议修改应答 */
void RS00_ParseProtocolChangeReply(uint8_t motor_id, uint8_t *data, uint8_t dlc);

#endif
