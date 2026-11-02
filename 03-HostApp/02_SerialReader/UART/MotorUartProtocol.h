#ifndef MOTOR_UART_PROTOCOL_H
#define MOTOR_UART_PROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QVector>
#include <QVariant>
#include <QMutex>

// Motor address definitions
enum class MotorAddress : uint8_t {
    MOTOR_1 = 0x01,
    MOTOR_2 = 0x02,
    MOTOR_3 = 0x03
};

// Command type definitions
enum class CommandType : uint8_t {
    START_STOP = 0x01,   // Start/stop command
    RESET = 0x02,        // Reset command
    ANGLE = 0x03,        // Angle command
    ANGULAR_VEL = 0x04,  // Angular velocity command
    STATUS_REQ = 0x05,   // Status request
    ACK = 0x06,          // Acknowledge response
    ERROR = 0x07,        // Error response
    PARA_1 = 0x08,       // Parameter 1 command
    PARA_2 = 0x09,       // Parameter 2 command
    PARA_3 = 0x0A,       // Parameter 3 command
    // Motor protocol specific commands (0x10-0x1F)
    MOTOR_ANGLE_FLOAT = 0x10,  // Float angle command (for motor protocol)
    MOTOR_VEL_FLOAT = 0x11,    // Float velocity command (for motor protocol)
    MOTOR_CURRENT_FLOAT = 0x12, // Float current command (for motor protocol)
    MOTOR_KP_FLOAT = 0x13,     // Float KP command (for motor protocol)
    MOTOR_KI_FLOAT = 0x14,     // Float KI command (for motor protocol)
    MOTOR_B0_FLOAT = 0x15,     // Float B0 command (for motor protocol)
    MOTOR_RESERVED_7 = 0x16,   // Reserved
    MOTOR_RESERVED_8 = 0x17    // Reserved
};

// Data type definitions
enum class DataType : uint8_t {
    NONE = 0x00,        // No data
    INT32 = 0x01,       // 32-bit integer
    FLOAT = 0x02        // Single precision float
};

// Motor status structure
struct MotorStatus {
    float position;          // Position
    float velocity;          // Velocity
    float temperature;       // Temperature
    bool online;             // Online status

    MotorStatus() : position(0.0f), velocity(0.0f), temperature(0.0f), online(false) {}
};

class MotorUartProtocol : public QObject
{
    Q_OBJECT
public:
    explicit MotorUartProtocol(QObject *parent = nullptr);

    // Command building functions
    QByteArray buildCommandFrame(MotorAddress addr, CommandType cmd, DataType data_type, const QVariant &data = QVariant());
    QByteArray buildStartStopCommand(MotorAddress addr, bool start);
    QByteArray buildMotorResetCommand(MotorAddress addr);
    QByteArray buildAngleCommand(MotorAddress addr, float angle);
    QByteArray buildAngularVelocityCommand(MotorAddress addr, float angular_vel);
    QByteArray buildStatusRequest(MotorAddress addr);
    QByteArray buildPara1Command(MotorAddress addr, float value);
    QByteArray buildPara2Command(MotorAddress addr, float value);
    QByteArray buildPara3Command(MotorAddress addr, float value);
    
    // New motor protocol commands based on the provided protocol table
    QByteArray buildMotorControlFrame(uint8_t cmd1, uint8_t cmd2, float floatValue = 0.0f);
    QByteArray buildMotorStartStopCommand(MotorAddress addr, bool start);
    QByteArray buildMotorAngleCommand(MotorAddress addr, float angle);
    QByteArray buildMotorVelocityCommand(MotorAddress addr, float velocity);
    QByteArray buildMotorCurrentCommand(MotorAddress addr, float current);
    QByteArray buildMotorKPCommand(MotorAddress addr, float kp);
    QByteArray buildMotorKICommand(MotorAddress addr, float ki);
    QByteArray buildMotorB0Command(MotorAddress addr, float b0);
    
    // MIT mode selection for motor 2
    QByteArray buildMotorMITModeCommand(uint8_t mode);
    // Zero point setting for motor 2
    QByteArray buildMotorZeroSetCommand();
    // Clear error for motor 2
    QByteArray buildMotorClearErrorCommand();
    // MIT dynamic parameters command for motor 2
    QByteArray buildMotorMITDynamicParamsCommand(float targetAngle, float targetVelocity, float kp, float kd, float targetTorque);
    // Position mode control command for motor 2
    QByteArray buildMotorPositionModeCommand(float targetAngle, float targetVelocity);
    // Velocity mode control command for motor 2
    QByteArray buildMotorVelocityModeCommand(float targetVelocity, float currentLimit);
    
    // Helper function for float to byte conversion (STM32 format - little endian)
    QByteArray floatToStm32Bytes(float value);

    // YAW, ROLL, PITCH axis parameter commands
    QByteArray buildYawParaCommand(uint8_t paramIndex, float value);
    QByteArray buildRollParaCommand(uint8_t paramIndex, float value);
    QByteArray buildPitchParaCommand(uint8_t paramIndex, float value);
    
    // FLASH update command
    QByteArray buildFlashUpdateCommand();

    // Gimbal control commands
    QByteArray buildPitchUpCommand(uint8_t speed);
    QByteArray buildPitchDownCommand(uint8_t speed);
    QByteArray buildYawLeftCommand(uint8_t speed);
    QByteArray buildYawRightCommand(uint8_t speed);
    QByteArray buildPitchAngleCommand(int16_t angle);
    QByteArray buildYawAngleCommand(int16_t angle);
    QByteArray buildResetCommand();
    QByteArray buildLookDownCommand();
    QByteArray buildYawFollowCommand();
    QByteArray buildYawLockCommand();
    QByteArray buildStopCommand();
    // ★ Passive Rehabilitation commands (cmd1=0x30)
    QByteArray buildPassiveStart(float speed, uint32_t cycles);
    QByteArray buildPassiveStop();
    QByteArray buildPassiveSetRange(float minAngle, float maxAngle);
    QByteArray buildPassiveSetPause(uint32_t ms);
    QByteArray buildPassiveSetDirection(uint8_t dir);

    // Parse data frame
    bool parseRxFrame(const QByteArray &data, QVector<MotorStatus> &motorStatuses);

    // Get parsed motor statuses
    QVector<MotorStatus> getMotorStatuses() const;

private:
    uint8_t calculateChecksum(const QByteArray &data);

signals:
    void dataParsed(const QVector<MotorStatus> &motorStatuses);
    void parseError(const QString &error);

private:
    mutable QMutex m_mutex; // Mutex for thread safety (mutable to allow locking in const methods)
    QVector<MotorStatus> m_motorStatuses;  // Motor status cache
};

#endif // MOTOR_UART_PROTOCOL_H