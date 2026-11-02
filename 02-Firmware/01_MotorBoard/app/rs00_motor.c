#include "rs00_motor.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define PI_F 3.1415926535f
#define DEG_TO_RAD (PI_F / 180.0f)
#define RAD_TO_DEG (180.0f / PI_F)

// ????:???? 60 ? = 1.0472 ??
#define MAX_LIMIT_RAD (60.0f * DEG_TO_RAD)
float g_relative_new_zero = 0.0f;
 int8_t g_homing_stage = HOMING_RUNNING;
RS00_FeedbackTypeDef motor_feedback;

/**
 * @brief 将16位数据转换回浮点数
 * @param x 待转换的原始数据
 * @param x_min/x_max 变量的物理范围
 */
static float RS00_UintToFloat(uint16_t x, float x_min, float x_max)
{
    return ((float)x) / 65535.0f * (x_max - x_min) + x_min;
}

/**
 * @brief 将浮点数压缩为16位或32位整数
 */
int RS00_FloatToUInt(float x, float x_min, float x_max, int bits) 
{
    if (x > x_max) x = x_max;
    else if (x < x_min) x = x_min;
    
    float span = x_max - x_min;
    
    return (int)((x - x_min) / span * ((1 << bits) - 1) );
}

/**
 * @brief 安全的 CAN 发送函数
 * @note 检查发送邮箱是否空闲，防止总线拥堵导致发送失败
 */
static HAL_StatusTypeDef Safe_CAN_Transmit(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *pHeader, uint8_t aData[])
{
    uint32_t txMailbox;
    uint32_t timeout_cnt = 0;

   // 等待邮箱空闲，若长时间无法发送则报错
    while (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0)
    {
        timeout_cnt++;
        if (timeout_cnt > 0x1FFFFF) // ? CPU ??????(????)
        {
            printf("CAN TX busy: send timeout, check wiring\r\n");
            return HAL_ERROR;
        }
    }
    return HAL_CAN_AddTxMessage(hcan, pHeader, aData, &txMailbox);
}

/* 电机初始化 */
void RS00_CAN_Init(void) {
     if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        printf("CAN start failed!\n");
    } else {
        printf("CAN started successfully.\n");
    }
}

/* 使能 */
void RS00_Enable(uint8_t motor_id, uint16_t master_id) {
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};
    
    txHeader.StdId = 0;
    txHeader.ExtId = ((3 << 24) | (master_id << 8) | motor_id);  // master_id ???
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_EXT;
    txHeader.DLC = 8;
    
//    printf("extid = %x\n", txHeader.ExtId);

    Safe_CAN_Transmit(&hcan1, &txHeader, txData);
}

/* 停机 */
void RS00_Stop(uint8_t motor_id, uint8_t master_id) {
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.ExtId = ((0x04 << 24) | (master_id << 8) | motor_id); 
    txHeader.DLC = 8;
    
    Safe_CAN_Transmit(&hcan1, &txHeader, txData);
}

/* 停机并清除错误标志 */
void RS00_StopAndClearError(uint8_t motor_id, uint8_t master_id)
{
	CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.ExtId = ((0x04 << 24) | (master_id << 8) | motor_id);
    txHeader.DLC = 8;

    // 数据位1为置位，用于清除错误
    txData[0] = 0x01;

    Safe_CAN_Transmit(&hcan1, &txHeader, txData);
}






/* 通信类型0: 获取设备ID */
void RS00_ReadDeviceID(uint8_t motor_id, uint8_t master_id)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;

    // 扩展帧ID组成: (通信类型[28:24] << 24) | (主控ID[23:8] << 8) | (目标电机ID[7:0])
    txHeader.ExtId = ((0x00 << 24) | (master_id << 8) | (motor_id));

    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) != HAL_OK)
    {
        printf("CAN TX failed!\r\n");
    }
    else
    {
        printf("Send RS00 Device ID request, MotorID: 0x%02X\r\n", motor_id);
    }
}

/* 获取电机反馈数据帧解析 */
void RS00_GetFeedbackFrame(uint8_t *rxData, RS00_FeedbackTypeDef *fb)
{
//    for (int i = 0; i < 8; i++)
//        printf("rxData[%d] = %X ", i, rxData[i]);
////    printf("\r\n");
    // 将收到的8字节数据按大端模式转换为物理量
    uint16_t raw_pos  = (rxData[0] << 8) | rxData[1];
    uint16_t raw_vel  = (rxData[2] << 8) | rxData[3];
    uint16_t raw_tor  = (rxData[4] << 8) | rxData[5];
    uint16_t raw_temp = (rxData[6] << 8) | rxData[7];

    fb->position    = RS00_UintToFloat(raw_pos, -12.57f, 12.57f);
    fb->speed       = RS00_UintToFloat(raw_vel, -33.0f, 33.0f);
    fb->torque      = RS00_UintToFloat(raw_tor, -14.0f, 14.0f);
    fb->temperature = ((float)raw_temp) / 10.0f;
}


/* 读取状态*/
void RS00_ReadMotorState(uint8_t motor_id, uint8_t master_id)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    txHeader.ExtId = ((0x02 << 24) | (master_id << 8) | motor_id);

    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) == HAL_OK)
        printf("Request motor state (MotorID=0x%02X)\r\n", motor_id);
    else
        printf("CAN TX failed when requesting motor state.\r\n");
}


void RS00_SetZeroPosition(uint8_t motor_id, uint8_t master_id)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;

    txHeader.ExtId = ((0x06 << 24) | (master_id << 8) | motor_id);
	txData[0] = 0x01;
    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) == HAL_OK)
        printf("Send SET ZERO POSITION command to Motor[0x%02X]\r\n", motor_id);
    else
        printf("CAN TX failed: Set Zero Position\r\n");
}

/* ===================== ?????? (Type = 0x01) ===================== */
void RS00_Control(uint8_t motor_id, uint8_t master_id, float position, float speed, float torque, float kp, float kd)
{
    (void)master_id;
//	printf("DEBUG: Control packet sent from line %d\n", __LINE__);
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8];

    // 1. 将浮点数转换为 16 位整数 (手册定义了映射范围)
    uint16_t torque_i = (uint16_t)RS00_FloatToUInt(torque, RS00_T_MIN, RS00_T_MAX, 16);

   //位置速度
    uint16_t pos_i   = (uint16_t)RS00_FloatToUInt(position, RS00_P_MIN, RS00_P_MAX, 16);
    uint16_t speed_i = (uint16_t)RS00_FloatToUInt(speed, RS00_V_MIN, RS00_V_MAX, 16);

    // KP、KD 
    uint16_t kp_i = (uint16_t)RS00_FloatToUInt(kp, RS00_KP_MIN, RS00_KP_MAX, 16);
    uint16_t kd_i = (uint16_t)RS00_FloatToUInt(kd, RS00_KD_MIN, RS00_KD_MAX, 16);

    txHeader.IDE   = CAN_ID_EXT;
    txHeader.RTR   = CAN_RTR_DATA;
    txHeader.DLC   = 8;
    txHeader.ExtId = ((0x01 << 24) | (torque_i << 8) | motor_id);

    // 2. 填充 CAN 报文
    txData[0] = (pos_i >> 8) & 0xFF;
    txData[1] = pos_i & 0xFF;
    txData[2] = (speed_i >> 8) & 0xFF;
    txData[3] = speed_i & 0xFF;
    txData[4] = (kp_i >> 8) & 0xFF;
    txData[5] = kp_i & 0xFF;
    txData[6] = (kd_i >> 8) & 0xFF;
    txData[7] = kd_i & 0xFF;
    
//    for (int i = 0; i < 8; i++)
//        printf("txData[%d] = %X  ", i, txData[i]);
////    printf("\n");

    // 3. 设置帧头 
    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) == HAL_OK)
    {
//        printf("Send CONTROL (Type=0x01) to Motor[0x%02X]: Torque=%.3f, Pos=%.3f, Vel=%.3f, Kp=%.3f, Kd=%.3f\r\n",
//               motor_id, torque, position, speed, kp, kd);
    }
    else
    {
        printf("CAN TX failed: CONTROL command\r\n");
    }
}

// ============================== 基础参数写入 ==============================
/**
 * @brief  通过 CAN 写入 1 字节参数（uint8_t 类型）
 * @param  motor_id  电机 ID (低 8 位)
 * @param  master_id 主机 ID (次低 8 位)
 * @param  index     参数索引 (16 位)
 * @param  value     写入的 8 位无符号值
 */
void RS00_WriteParamUint8(uint8_t motor_id, uint8_t master_id, uint16_t index, uint8_t value)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8];
 // 构造扩展帧 ID: [CMD_TYPE(8bit)] [master_id(8bit)] [motor_id(8bit)] (高位在前)
    txHeader.ExtId = ((CMD_TYPE_WRITE_PARAM << 24) | (master_id << 8) | motor_id);
    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    memset(txData, 0, 8);
    
     // 写入参数索引（小端序）
    txData[0] = index & 0xFF;	 // 索引低字节
    txData[1] = (index >> 8) & 0xFF;	  // 索引高字节
    txData[4] = value;   // 1 字节参数值放在字节 4
    
    Safe_CAN_Transmit(&hcan1, &txHeader, txData);
//    printf("Write Uint8: index=0x%04X, value=%u\n", index, value);
}

/**
 * @brief  通过 CAN 写入 32 位浮点数参数（float 类型）
 * @param  motor_id  电机 ID
 * @param  master_id 主机 ID
 * @param  index     参数索引
 * @param  value     写入的浮点数值
 */
void RS00_WriteParamFloat(uint8_t motor_id, uint8_t master_id, uint16_t index, float value)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8];

    txHeader.ExtId = ((CMD_TYPE_WRITE_PARAM << 24) | (master_id << 8) | motor_id);
    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    memset(txData, 0, 8);
    
    // 填入参数索引
    txData[0] = index & 0xFF;
    txData[1] = (index >> 8) & 0xFF;
    
     // 将 float 转换为 4 字节数组（小端序）
    uint8_t *value_bytes = (uint8_t*)&value;
    for(int i = 0; i < 4; i++) {
        txData[4 + i] = value_bytes[i];
    }
    
    Safe_CAN_Transmit(&hcan1, &txHeader, txData);
//    printf("Write Float: index=0x%04X, value=%.3f\n", index, value);
}


/**
 * @brief  配置电机进入速度模式（初始化或切换模式时调用一次即可）
 */
void RS00_ConfigSpeedMode(uint8_t motor_id, uint8_t master_id)
{
    RS00_WriteParamUint8(motor_id, master_id, 0x7005, 2);
}

/**
 * @brief  高频给定电机目标速度（在控制流、定时器或主循环中连续调用）
 */
void RS00_WriteSpeed(uint8_t motor_id, uint8_t master_id, float target_speed)
{
    RS00_WriteParamFloat(motor_id, master_id, 0x700A, target_speed);
}
/**
 * @brief  RS00 电机纯速度模式控制
 * @param  motor_id     目标电机 ID
 * @param  master_id    主机 ID
 * @param  target_speed 目标速度 (float, 单位为 rad/s)
 */
void RS00_SetSpeedMode_Control(uint8_t motor_id, uint8_t master_id, float target_speed)
{
    // 1. 设置运行模式为速度模式 (index=0x7005, value=2) -> 对应 0570000002000000
    RS00_WriteParamUint8(motor_id, master_id, 0x7005, 2);
    
    // 2. 写入目标速度到速度期望寄存器 (index=0x700A, value=Speed) -> 对应 0A700000xxxxxx
    RS00_WriteParamFloat(motor_id, master_id, 0x700A, target_speed);
}

/* ===================== 模式与参数设置 ===================== */
/**
 * @brief  设置电机运行模式
 * @param  run_mode  0: 运控模式, 1: 位置模式, 2: 速度模式, 3: 电流模式
 */
void RS00_SetRunMode(uint8_t motor_id, uint8_t master_id, uint8_t run_mode)
{
    RS00_WriteParamUint8(motor_id, master_id, PARAM_INDEX_RUN_MODE, run_mode);
}

/* ===================== 位置模式参数（PP 模式） ===================== */
/**
 * @brief  配置位置模式下的最大速度和加速度
 */
void RS00_SetPositionModeParams(uint8_t motor_id, uint8_t master_id, float max_velocity, float acceleration)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_VEL_MAX, max_velocity);
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_ACC_SET, acceleration);
}

/* ===================== 速度模式参数 ===================== */
/**
 * @brief  配置速度模式下的最大电流和加速度
 */
void RS00_SetVelocityModeParams(uint8_t motor_id, uint8_t master_id, float max_current, float acceleration)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_LIMIT_CUR, max_current);
    // ??????? HAL_Delay(10),????
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_ACC_RAD, acceleration);
}

/* ===================== 位置/速度指令 ===================== */
/**
 * @brief  发送目标位置指令
 */
void RS00_SendPositionCommand(uint8_t motor_id, uint8_t master_id, float position)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_LOC_REF, position);
}

/**
 * @brief  发送目标速度指令
 */
void RS00_SendVelocityCommand(uint8_t motor_id, uint8_t master_id, float velocity)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_SPD_REF, velocity);
}

/* ===================== PID / 控制增益设置 ===================== */

/**
 * @brief  设置位置环比例增益
 */
void RS00_SetPositionControlParams(uint8_t motor_id, uint8_t master_id, float kp)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_LOC_KP, kp);
    printf("Set position KP: %.3f\n", kp);
}

/**
 * @brief  设置速度环 PID 参数及滤波增益
 */
void RS00_SetSpeedControlParams(uint8_t motor_id, uint8_t master_id, float kp, float ki, float filter_gain)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_SPD_KP, kp);
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_SPD_KI, ki);
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_SPD_FILTER_GAIN, filter_gain);
    printf("Set speed control: KP=%.3f, KI=%.3f, Filter=%.3f\n", kp, ki, filter_gain);
}

/**
 * @brief  设置电流环 PID 参数及滤波增益
 */
void RS00_SetCurrentControlParams(uint8_t motor_id, uint8_t master_id, float kp, float ki, float filter_gain)
{
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_CUR_KP, kp);
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_CUR_KI, ki);
    RS00_WriteParamFloat(motor_id, master_id, PARAM_INDEX_CUR_FILTER_GAIN, filter_gain);
    printf("Set current control: KP=%.3f, KI=%.3f, Filter=%.3f\n", kp, ki, filter_gain);
}

/**
 * @brief  发送读参数请求（在 rs00_motor.c 中实现回调解析）
 */
void RS00_ReadParam(uint8_t motor_id, uint8_t master_id, uint16_t index)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8];

    txHeader.ExtId = ((CMD_TYPE_READ_PARAM << 24) | (master_id << 8) | motor_id);
    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    
    memset(txData, 0, 8);
    
    // 填入要读取的参数索引
    txData[0] = index & 0xFF;
    txData[1] = (index >> 8) & 0xFF;
    
    Safe_CAN_Transmit(&hcan1, &txHeader, txData);
    printf("Read param request: index=0x%04X\n", index);
}

/* ===================== CAN 帧解析：读参数回复 ===================== */
/**
 * @brief  解析电机返回的“读参数”应答帧
 * @param  data  数据段指针
 * @param  dlc   数据长度
 */
void RS00_ParseReadParamReply(uint8_t motor_id, uint8_t *data, uint8_t dlc)
{
    printf("  [Read Param] MotorID=0x%02X\r\n", motor_id);
    printf("    Raw Data: ");
    for (int i = 0; i < dlc; i++) printf("%02X ", data[i]);
    printf("\r\n");
    
    if (dlc >= 8)
    {
       // 参数索引（小端序）
        uint16_t param_index = (data[1] << 8) | data[0];
        
         // 状态标志 (Bit23~16): 0x00 成功, 0x01 失败
        uint8_t status = (data[2] >> 16) & 0xFF;  // Bit23~16: 00???,01???
        
        if (status == 0x00)
        {
            // 参数值（4 字节，小端序）
            uint32_t param_raw = 0;
            for (int i = 0; i < 4; i++) {
                param_raw |= (data[4 + i] << (i * 8));
            }
            
           // 将原始字节转换为 float
            float param_value;
            memcpy(&param_value, &param_raw, 4);
            
            printf("    Parameter Index: 0x%04X\r\n", param_index);
            printf("    Parameter Value: %.6f (0x%08X)\r\n", param_value, param_raw);
            
            // ????????????
            const char *param_name = "Unknown";
            switch (param_index)
            {
                case 0x7005: param_name = "RUN_MODE "; break;//(运行模式)
                case 0x7006: param_name = "IQ_REF "; break;//(电流参考)
                case 0x700A: param_name = "SPD_REF "; break;//(速度参考)
                case 0x700B: param_name = "LIMIT_TORQUE "; break;//(扭矩限制)
                case 0x7010: param_name = "CUR_KP "; break;//(电流KP)
                case 0x7011: param_name = "CUR_KI "; break;//(电流KI)
                case 0x7014: param_name = "CUR_FILTER_GAIN "; break;//(电流滤波增益)
                case 0x7016: param_name = "LOC_REF"; break; //(位置参考)
                case 0x7017: param_name = "LIMIT_SPD"; break; //(速度限制)
                case 0x7018: param_name = "LIMIT_CUR "; break;//(电流限制)
                case 0x7019: param_name = "MECH_POS "; break;//(机械位置)
                case 0x701A: param_name = "IQ_FILTERED "; break;//(滤波后Iq)
                case 0x701B: param_name = "MECH_VEL "; break;//(机械速度)
                case 0x701C: param_name = "VBUS "; break;//(母线电压)
                case 0x701E: param_name = "LOC_KP "; break;//(位置KP)
                case 0x701F: param_name = "SPD_KP"; break; //(速度KP)
                case 0x7020: param_name = "SPD_KI "; break;//(速度KI)
                case 0x7021: param_name = "SPD_FILTER_GAIN "; break;//(速度滤波增益)
                case 0x7022: param_name = "ACC_RAD "; break;//(加速度)
                case 0x7024: param_name = "VEL_MAX "; break;//(最大速度)
                case 0x7025: param_name = "ACC_SET "; break;//(加速度设置)
                case 0x7026: param_name = "EPSCAN_TIME "; break;//(编码器扫描时间)
                case 0x7028: param_name = "CAN_TIMEOUT "; break;//(CAN超时)
                case 0x7029: param_name = "ZERO_STA "; break;//(零点状态)
                case 0x702A: param_name = "DAMPER "; break;//(阻尼器)
                case 0x702B: param_name = "ADD_OFFSET"; break; //(附加偏移)
            }
            printf("    Parameter Name: %s\r\n", param_name);
        }
        else
        {
            printf("    Parameter Read Failed! Status: 0x%02X\r\n", status);
        }
    }
    else
    {
        printf("    Invalid data length for parameter read reply\r\n");
    }
}

/* ===================== CAN 帧解析：写参数应答 ===================== */
/**
 * @brief  解析电机返回的“写参数”应答帧
 */
void RS00_ParseWriteParamAck(uint8_t motor_id, uint8_t *data, uint8_t dlc)
{
    (void)motor_id;
//    printf("  [Write Param Ack] MotorID=0x%02X\r\n", motor_id);
    printf("    Raw Data: ");
    for (int i = 0; i < dlc; i++) printf("%02X ", data[i]);
    printf("\r\n");
    
   // 通常前 2 字节为参数索引，此处仅确认写入成功
    if (dlc >= 8)
    {
        printf("    Write operation acknowledged\r\n");
    }
}

/* ===================== CAN 帧解析：故障帧 ===================== */
/**
 * @brief  解析故障反馈帧（通信类型 21）
 * @note   字节 0~3 为故障码，字节 4~7 为警告码
 */
void RS00_ParseFaultFrame(uint8_t motor_id, uint8_t *data, uint8_t dlc)
{
    printf("  [Fault Feedback] MotorID=0x%02X\r\n", motor_id);
    
    if (dlc >= 4)
    {
        uint32_t fault_value = 0;
        for (int i = 0; i < 4; i++) {
            fault_value |= (data[i] << (i * 8));
        }
        printf("    Fault Code: 0x%08X\r\n", fault_value);
        
        // ?????(??????21?)
        if (fault_value == 0) {
            printf("    Status: Normal (no fault)\r\n");
        } else {
            printf("    Fault Details:\r\n");
           if (fault_value & (1 << 0)) printf("      - Overtemp fault (over 145C)\r\n");
            if (fault_value & (1 << 1)) printf("      - 过流故障\r\n");
            if (fault_value & (1 << 2)) printf("      - 欠压故障 (低于12V)\r\n");
            if (fault_value & (1 << 3)) printf("      - 过压故障 (高于60V)\r\n");
            if (fault_value & (1 << 4)) printf("      - B相电流采样故障\r\n");
            if (fault_value & (1 << 5)) printf("      - C相电流采样故障\r\n");
            if (fault_value & (1 << 7)) printf("      - 编码器故障\r\n");
            if (fault_value & (1 << 8)) printf("      - 过载故障\r\n");
            if (fault_value & (1 << 9)) printf("      - 速度超限故障\r\n");
            if (fault_value & (1 << 14)) printf("      - 位置偏差过大故障\r\n");
            if (fault_value & (1 << 16)) printf("      - A相电流采样故障\r\n");
        }
    }
    
    // ???warning?(Byte4~7)
    if (dlc >= 8)
    {
        uint32_t warning_value = 0;
        for (int i = 0; i < 4; i++) {
            warning_value |= (data[4 + i] << (i * 8));
        }
        
        if (warning_value != 0) {
            printf("    Warning Code: 0x%08X\r\n", warning_value);
            if (warning_value & (1 << 0)) printf("     - Overtemp warning (over 135C)\r\n");
        }
    }
}

/* ===================== CAN 帧解析：主动上报帧 ===================== */
/**
 * @brief  解析主动上报帧（通信类型 24），包含位置、速度、力矩、温度
 */
void RS00_ParseActiveReport(uint8_t motor_id, uint8_t *data, uint8_t dlc)
{
    printf("  [Active Report] MotorID=0x%02X\r\n", motor_id);
    
     // 每项数据占 2 字节，高字节在前
    if (dlc >= 8)
    {
      
        uint16_t raw_pos  = (data[0] << 8) | data[1];
        uint16_t raw_vel  = (data[2] << 8) | data[3];
        uint16_t raw_tor  = (data[4] << 8) | data[5];
        uint16_t raw_temp = (data[6] << 8) | data[7];
         // 量纲转换
        float position = ((float)raw_pos) / 65535.0f * 25.14f - 12.57f;
        float speed = ((float)raw_vel) / 65535.0f * 66.0f - 33.0f;
        float torque = ((float)raw_tor) / 65535.0f * 28.0f - 14.0f;
        float temperature = ((float)raw_temp) / 10.0f;
        
        printf("    Position: %.3f rad\r\n", position);
        printf("    Speed: %.3f rad/s\r\n", speed);
        printf("    Torque: %.3f Nm\r\n", torque);
        printf("    Temperature: %.1f ?\r\n", temperature);
    }
}

/* ===================== CAN 帧总解析入口 ===================== */
/**
 * @brief  根据扩展帧 ID 识别通信类型，调用相应解析函数
 * @param  extid  29 位扩展 ID
 * @param  data   数据段
 * @param  dlc    数据长度
 */
void RS00_ParseCANFrame(uint32_t extid, uint8_t *data, uint8_t dlc)
{
    uint8_t comm_type = (extid >> 24) & 0x1F;	// 通信类型域
    uint8_t motor_id  = (extid >> 8) & 0xFF;   // 电机 ID
    
//    printf("\r\n=== [CAN RX Frame] ===\r\n");
//    printf("  ExtID: 0x%08X | Type: 0x%02X | MotorID: 0x%02X\r\n", 
//           extid, comm_type, motor_id);
//    printf("  Data[%d]: ", dlc);
//    for (int i = 0; i < dlc; i++) printf("%02X ", data[i]);
    //printf("\r\n");

    
    switch (comm_type)
    {
        case 0x00:  // 设备信息回复
//            printf("  Device Info Reply\r\n");
//            printf("    UID: ");
//            for (int i = 0; i < dlc; i++) printf("%02X", data[i]);
//            printf("\r\n");
            break;
        case 0x02:   // 电机状态反馈
//            printf("  Motor State Feedback\r\n");
            RS00_GetFeedbackFrame(data, &motor_feedback);
//            printf("    Position: %.3f rad\r\n", motor_feedback.position);
//            printf("    Speed: %.3f rad/s\r\n", motor_feedback.speed);
//            printf("    Torque: %.3f N�m\r\n", motor_feedback.torque);
//            printf("    Temperature: %.1f ?\r\n", motor_feedback.temperature);
            break;
        case 0x03:  // 使能应答
//            printf("  Motor Enable Acknowledged\r\n");
            break;
            
        case 0x04:   // 停止应答
            printf("  Motor Stop Acknowledged\r\n");
            break;
            
        case 0x06:   // 设置零点应答
            printf("  Set Zero Position Acknowledged\r\n");
            break;
            
        case 0x11:   // 读参数回复 (17)
            RS00_ParseReadParamReply(motor_id, data, dlc);
            break;
            
        case 0x12:   // 写参数应答 (18)
            RS00_ParseWriteParamAck(motor_id, data, dlc);
            break;
            
        case 0x15:   // 故障反馈 (21)
            RS00_ParseFaultFrame(motor_id, data, dlc);
            break;
        
        case 0x16:  // 保存参数应答 (22)
            printf("  Parameters Saved Acknowledged\r\n");
            break;
            
        case 0x17:   // 波特率切换应答 (23)
            printf("  Baud Rate Change Acknowledged\r\n");
            printf("    Note: Baud rate change requires motor restart.\r\n");
            break;
            
        case 0x18:   // 主动上报 (24)
            RS00_ParseActiveReport(motor_id, data, dlc);
            break;
        
        case 0x19:   // 协议切换应答 (25)
            RS00_ParseProtocolChangeReply(motor_id, data, dlc);
            break;
            
        default:
            printf("  Unknown Frame Type\r\n");          
            if (dlc > 0) {
                printf("    Raw Data: ");
                for (int i = 0; i < dlc; i++) {
                    printf("%02X ", data[i]);
                }
                printf("\r\n");
            }
            break;
    }
}

/* ===================== 协议切换与波特率设置 ===================== */

/**
 * @brief  设置电机通信协议类型（通信类型 25，需重启生效）
 * @param  protocol  0=私有协议, 1=Canopen, 2=MIT
 */
void RS00_SetProtocolType(uint8_t motor_id, uint8_t master_id, uint8_t protocol)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;

   
    txHeader.ExtId = ((0x19 << 24) | (master_id << 8) | motor_id);

    // 固定前缀 + 协议选择字节
    txData[0] = 0x01;
    txData[1] = 0x02;
    txData[2] = 0x03;
    txData[3] = 0x04;
    txData[4] = 0x05;
    txData[5] = 0x06;
    txData[6] = protocol;  // F_CMD：协议类型

    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) == HAL_OK)
    {
        printf("Set protocol type to %d (0=Private, 1=Canopen, 2=MIT)\r\n", protocol);
        printf("Note: Motor needs to be restarted for protocol change to take effect.\r\n");
    }
    else
    {
        printf("CAN TX failed: Set protocol type\r\n");
    }
}

/**
 * @brief  通过参数写入方式切换协议（操作 protocol_1 寄存器并保存）
 * @param  protocol  0=私有, 1=Canopen, 2=MIT
 */
void RS00_SetProtocolTypeByParam(uint8_t motor_id, uint8_t master_id, uint8_t protocol)
{
     // 写 protocol_1 参数 (0x7022)
    RS00_WriteParamUint8(motor_id, master_id, 0x7022, protocol);
    
     // 保存参数（通信类型 22）
    RS00_SaveParameters(motor_id, master_id);
    
    printf("Set protocol type to %d via parameter write\r\n", protocol);
    printf("Note: Motor needs to be restarted for protocol change to take effect.\r\n");
}


// 返回值：如果需要发送指令返回 1，否则返回 0
uint8_t RS00_Calculate_Homing(
    float *out_pos,
    float *out_kp,
    float *out_kd)
{
    float current = motor_feedback.position;

    float error =
        0.0f - current;

    printf(
        "current=%.3f  error=%.3f\r\n",
        current,
        error);

    if(fabsf(error) < 0.03f)
    {
        g_homing_stage = HOMING_SUCCESS;
        return 0;
    }

    if(fabsf(error) > MAX_LIMIT_RAD)
    {
        printf("Homing Error!\r\n");

        g_homing_stage = HOMING_ERROR;
        return 0;
    }

    *out_pos = g_relative_new_zero;

    *out_kp = 1.5f;
    *out_kd = 0.8f;

    return 1;
}

/**
 * @brief  最短路径柔性归零执行函数
 * @param  motor_id: 电机 CAN ID (如 0x7F)
 * @param  master_id: 主控 CAN ID (如 0xFD)
 */
void RS00_StartHoming(uint8_t motor_id,uint8_t master_id)
{
    (void)motor_id;
    (void)master_id;
    RS00_Control(
        0x7F,
        0xFD,
        1.0f,
        0.0f,
        0.0f,
        2.0f,
        1.0f);
}
float CalculateShortestTarget(float current)
{
    float target = 0.0f;

    float diff = target - current;

    while(diff > PI_F)
        diff -= 2.0f * PI_F;

    while(diff < -PI_F)
        diff += 2.0f * PI_F;

    return diff;
}

/**
 * @brief  发送保存参数指令（通信类型 22）
 */
void RS00_SaveParameters(uint8_t motor_id, uint8_t master_id)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;

    
    txHeader.ExtId = ((0x16 << 24) | (master_id << 8) | motor_id);

    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) == HAL_OK)
    {
        printf("Send save parameters command\r\n");
    }
    else
    {
        printf("CAN TX failed: Save parameters\r\n");
    }
}

/**
 * @brief  解析协议切换应答帧（通信类型 25）
 */
void RS00_ParseProtocolChangeReply(uint8_t motor_id, uint8_t *data, uint8_t dlc)
{
    printf("  [Protocol Change Reply] MotorID=0x%02X\r\n", motor_id);
    printf("    Raw Data: ");
    for (int i = 0; i < dlc; i++) printf("%02X ", data[i]);
    printf("\r\n");
    
    
    if (dlc >= 8)
    {
        printf("    Protocol change acknowledged\r\n");
        printf("    MCU UID: ");
        for (int i = 0; i < dlc; i++) printf("%02X", data[i]);
        printf("\r\n");
    }
}

/**
 * @brief  设置 CAN 波特率（通信类型 23，需重启）
 * @param  baud_rate  1:1M, 2:500K, 3:250K, 4:125K
 */
void RS00_SetBaudRate(uint8_t motor_id, uint8_t master_id, uint8_t baud_rate)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;  
    txHeader.ExtId = ((0x17 << 24) | (master_id << 8) | motor_id);  
    txData[6] = baud_rate;  //  // 波特率选择字节

    const char* baud_str = "Unknown";
    switch(baud_rate)
    {
        case 1: baud_str = "1Mbps"; break;
        case 2: baud_str = "500Kbps"; break;
        case 3: baud_str = "250Kbps"; break;
        case 4: baud_str = "125Kbps"; break;
    }

    if (Safe_CAN_Transmit(&hcan1, &txHeader, txData) == HAL_OK)
    {
        printf("Set baud rate to %d (%s)\r\n", baud_rate, baud_str);
        printf("Note: Baud rate change requires motor restart.\r\n");
    }
    else
    {
        printf("CAN TX failed: Set baud rate\r\n");
    }
}
