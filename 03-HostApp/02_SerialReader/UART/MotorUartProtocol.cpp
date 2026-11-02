#include "MotorUartProtocol.h"
#include <QDataStream>
#include <QDebug>
#include <QVariant>
#include <QIODevice>

MotorUartProtocol::MotorUartProtocol(QObject *parent) : QObject(parent)
{
    m_motorStatuses.resize(3); // Initialize status for 3 motors
}

QByteArray MotorUartProtocol::buildCommandFrame(MotorAddress addr, CommandType cmd, DataType data_type, const QVariant &data)
{
    QByteArray frame;

    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian); // Set little-endian mode

    // -------------------------- 1. Frame header (0xAA)--------------------------
    stream << static_cast<uint8_t>(0xAA);

    // -------------------------- 2. Valid data (address + command + data type + data, total 7 bytes)--------------------------
    // Record the start position of valid data (skip frame header 0xAA, start from index 1)
    int validDataStart = frame.size();

    // Address, command type and data type
    stream << static_cast<uint8_t>(addr)
           << static_cast<uint8_t>(cmd)
           << static_cast<uint8_t>(data_type);

    // Data part (4 bytes, fill with 0 when no data)
    if (data_type != DataType::NONE && data.isValid()) {
        if (data_type == DataType::FLOAT) {
            float float_data = data.toFloat();
            // Convert float to 4 bytes
            union {
                float f;
                uint8_t b[4];
            } converter;
            converter.f = float_data;

            // Little-endian order, low byte first
            for (int i = 0; i < 4; ++i) {
                stream << converter.b[i];
            }
        } else if (data_type == DataType::INT32) {
            int32_t int_data = data.toInt();
            // Little-endian order, low byte first
            for (int i = 0; i < 4; ++i) {
                stream << static_cast<uint8_t>((int_data >> (i * 8)) & 0xFF);
            }
        }
    } else {
        // No data, fill with 4 bytes of 0
        for (int i = 0; i < 4; ++i) {
            stream << static_cast<uint8_t>(0x00);
        }
    }

    // -------------------------- 3. Calculate checksum (exclude frame header 0xAA, only check valid data)--------------------------
    // Extract valid data (from validDataStart to end of frame, total 7 bytes)
    QByteArray validData = frame.mid(validDataStart);
    uint8_t checksum = calculateChecksum(validData); // Reuse existing checksum calculation function

    // -------------------------- 4. Insert checksum (before frame end 0x55)--------------------------
    stream << checksum;

    // -------------------------- 5. Frame end (0x55)--------------------------
    stream << static_cast<uint8_t>(0x55);

    return frame;
}

QByteArray MotorUartProtocol::buildStartStopCommand(MotorAddress addr, bool start)
{
    QByteArray result = buildCommandFrame(addr, CommandType::START_STOP, DataType::INT32, QVariant(start ? 1 : 2));
    return result;
}

QByteArray MotorUartProtocol::buildMotorResetCommand(MotorAddress addr)
{
    return buildCommandFrame(addr, CommandType::RESET, DataType::NONE);
}

QByteArray MotorUartProtocol::buildAngleCommand(MotorAddress addr, float angle)
{
    return buildCommandFrame(addr, CommandType::ANGLE, DataType::FLOAT, QVariant(angle));
}

QByteArray MotorUartProtocol::buildAngularVelocityCommand(MotorAddress addr, float angular_vel)
{
    return buildCommandFrame(addr, CommandType::ANGULAR_VEL, DataType::FLOAT, QVariant(angular_vel));
}

QByteArray MotorUartProtocol::buildStatusRequest(MotorAddress addr)
{
    return buildCommandFrame(addr, CommandType::STATUS_REQ, DataType::NONE);
}

QByteArray MotorUartProtocol::buildPara1Command(MotorAddress addr, float value)
{
    return buildCommandFrame(addr, CommandType::PARA_1, DataType::FLOAT, value);
}

QByteArray MotorUartProtocol::buildPara2Command(MotorAddress addr, float value)
{
    return buildCommandFrame(addr, CommandType::PARA_2, DataType::FLOAT, value);
}

QByteArray MotorUartProtocol::buildPara3Command(MotorAddress addr, float value)
{
    return buildCommandFrame(addr, CommandType::PARA_3, DataType::FLOAT, value);
}

// Build YAW axis parameter command
QByteArray MotorUartProtocol::buildYawParaCommand(uint8_t paramIndex, float value)
{
    // Limit value between 0-600
    if (value < 0) value = 0;
    if (value > 600) value = 600;
    
    // Convert to unsigned 16-bit number: value = (data1 << 8 | data2) / 100
    uint16_t dataValue = static_cast<uint16_t>(value * 100);
    
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0xF0)); // Command code 1 (YAW axis)
    frame.append(static_cast<char>(paramIndex)); // Command code 2 (parameter index)
    frame.append(static_cast<char>((dataValue >> 8) & 0xFF)); // Data code 1 (high byte)
    frame.append(static_cast<char>(dataValue & 0xFF)); // Data code 2 (low byte)
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0xF0 + paramIndex + static_cast<uint8_t>((dataValue >> 8) & 0xFF) + static_cast<uint8_t>(dataValue & 0xFF);
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build ROLL axis parameter command
QByteArray MotorUartProtocol::buildRollParaCommand(uint8_t paramIndex, float value)
{
    // Limit value between 0-600
    if (value < 0) value = 0;
    if (value > 600) value = 600;
    
    // Convert to unsigned 16-bit number: value = (data1 << 8 | data2) / 100
    uint16_t dataValue = static_cast<uint16_t>(value * 100);
    
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0xF1)); // Command code 1 (ROLL axis)
    frame.append(static_cast<char>(paramIndex)); // Command code 2 (parameter index)
    frame.append(static_cast<char>((dataValue >> 8) & 0xFF)); // Data code 1 (high byte)
    frame.append(static_cast<char>(dataValue & 0xFF)); // Data code 2 (low byte)
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0xF1 + paramIndex + static_cast<uint8_t>((dataValue >> 8) & 0xFF) + static_cast<uint8_t>(dataValue & 0xFF);
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build PITCH axis parameter command
QByteArray MotorUartProtocol::buildPitchParaCommand(uint8_t paramIndex, float value)
{
    // Limit value between 0-600
    if (value < 0) value = 0;
    if (value > 600) value = 600;
    
    // Convert to unsigned 16-bit number: value = (data1 << 8 | data2) / 100
    uint16_t dataValue = static_cast<uint16_t>(value * 100);
    
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0xF2)); // Command code 1 (PITCH axis)
    frame.append(static_cast<char>(paramIndex)); // Command code 2 (parameter index)
    frame.append(static_cast<char>((dataValue >> 8) & 0xFF)); // Data code 1 (high byte)
    frame.append(static_cast<char>(dataValue & 0xFF)); // Data code 2 (low byte)
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0xF2 + paramIndex + static_cast<uint8_t>((dataValue >> 8) & 0xFF) + static_cast<uint8_t>(dataValue & 0xFF);
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build FLASH update command
QByteArray MotorUartProtocol::buildFlashUpdateCommand()
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code (consistent with other commands)
    frame.append(static_cast<char>(0xE0)); // Command code 1 (update FLASH)
    frame.append(static_cast<char>(0x00)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0xE0 + 0x00 + 0x00 + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build pitch up command
QByteArray MotorUartProtocol::buildPitchUpCommand(uint8_t speed)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x00)); // Command code 1
    frame.append(static_cast<char>(0x08)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(speed)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x00 + 0x08 + 0x00 + speed;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build pitch down command
QByteArray MotorUartProtocol::buildPitchDownCommand(uint8_t speed)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x00)); // Command code 1
    frame.append(static_cast<char>(0x10)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(speed)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x00 + 0x10 + 0x00 + speed;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build yaw left command
QByteArray MotorUartProtocol::buildYawLeftCommand(uint8_t speed)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x00)); // Command code 1
    frame.append(static_cast<char>(0x04)); // Command code 2
    frame.append(static_cast<char>(speed)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x00 + 0x04 + speed + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build yaw right command
QByteArray MotorUartProtocol::buildYawRightCommand(uint8_t speed)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x00)); // Command code 1
    frame.append(static_cast<char>(0x02)); // Command code 2
    frame.append(static_cast<char>(speed)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x00 + 0x02 + speed + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build pitch angle control command
QByteArray MotorUartProtocol::buildPitchAngleCommand(int16_t angle)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x10)); // Command code 1
    frame.append(static_cast<char>(0x01)); // Command code 2
    
    // Convert angle to data code
    int16_t angleValue = angle * 50;
    frame.append(static_cast<char>((angleValue >> 8) & 0xFF)); // Data code 1
    frame.append(static_cast<char>(angleValue & 0xFF)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x10 + 0x01 + static_cast<uint8_t>((angleValue >> 8) & 0xFF) + static_cast<uint8_t>(angleValue & 0xFF);
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build yaw angle control command
QByteArray MotorUartProtocol::buildYawAngleCommand(int16_t angle)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x10)); // Command code 1
    frame.append(static_cast<char>(0x02)); // Command code 2
    
    // Convert angle to data code
    int16_t angleValue = angle * 50;
    frame.append(static_cast<char>((angleValue >> 8) & 0xFF)); // Data code 1
    frame.append(static_cast<char>(angleValue & 0xFF)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x10 + 0x02 + static_cast<uint8_t>((angleValue >> 8) & 0xFF) + static_cast<uint8_t>(angleValue & 0xFF);
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build reset command
QByteArray MotorUartProtocol::buildResetCommand()
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x13)); // Command code 1
    frame.append(static_cast<char>(0x03)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x13 + 0x03 + 0x00 + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build look down command
QByteArray MotorUartProtocol::buildLookDownCommand()
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x13)); // Command code 1
    frame.append(static_cast<char>(0x02)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x13 + 0x02 + 0x00 + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build yaw follow mode command
QByteArray MotorUartProtocol::buildYawFollowCommand()
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x13)); // Command code 1
    frame.append(static_cast<char>(0x00)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x13 + 0x00 + 0x00 + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build yaw lock mode command
QByteArray MotorUartProtocol::buildYawLockCommand()
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x13)); // Command code 1
    frame.append(static_cast<char>(0x01)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x13 + 0x01 + 0x00 + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build motion stop command
QByteArray MotorUartProtocol::buildStopCommand()
{
    QByteArray frame;
    frame.append(static_cast<char>(0xFF)); // Sync byte
    frame.append(static_cast<char>(0x01)); // Address code
    frame.append(static_cast<char>(0x00)); // Command code 1
    frame.append(static_cast<char>(0x00)); // Command code 2
    frame.append(static_cast<char>(0x00)); // Data code 1
    frame.append(static_cast<char>(0x00)); // Data code 2
    
    // Calculate checksum
    uint8_t checksum = 0x01 + 0x00 + 0x00 + 0x00 + 0x00;
    frame.append(static_cast<char>(checksum));
    
    return frame;
}


// ====================================================================
// ★ Passive Rehabilitation — 14-byte 0x6D/0x6A frame (cmd1=0x30)
// ====================================================================

// Helper: build generic passive rehab frame with 8 data bytes
static QByteArray buildPassiveFrame(uint8_t cmd2, const uint8_t dataBytes[8])
{
    QByteArray frame(14, Qt::Uninitialized);   // pre-allocate 14 bytes
    uint8_t *buf = reinterpret_cast<uint8_t *>(frame.data());

    buf[0] = 0x6D;                             // header 1
    buf[1] = 0x6A;                             // header 2
    buf[2] = 0x01;                             // address
    buf[3] = 0x30;                             // cmd1
    buf[4] = cmd2;                             // cmd2

    // Fill data bytes and compute checksum in one pass
    uint8_t sum = 0x01 + 0x30 + cmd2;          // bytes 2+3+4
    for (int i = 0; i < 8; ++i) {
        buf[5 + i] = dataBytes[i];
        sum += dataBytes[i];
    }
    buf[13] = sum;                             // checksum

    return frame;
}

// 0x30 0x01: Start passive rehab — speed(float) + cycles(uint32)
QByteArray MotorUartProtocol::buildPassiveStart(float speed, uint32_t cycles)
{
    uint8_t dataBytes[8] = {0};
    // speed as little-endian float in bytes 0-3
    union {
        float f;
        uint8_t b[4];
    } converter;
    converter.f = speed;
    for (int i = 0; i < 4; ++i) {
        dataBytes[i] = converter.b[i];
    }
    // cycles as little-endian uint32 in bytes 4-7
    dataBytes[4] = static_cast<uint8_t>(cycles & 0xFF);
    dataBytes[5] = static_cast<uint8_t>((cycles >> 8) & 0xFF);
    dataBytes[6] = static_cast<uint8_t>((cycles >> 16) & 0xFF);
    dataBytes[7] = static_cast<uint8_t>((cycles >> 24) & 0xFF);

    return buildPassiveFrame(0x01, dataBytes);
}

// 0x30 0x02: Stop passive rehab
QByteArray MotorUartProtocol::buildPassiveStop()
{
    uint8_t dataBytes[8] = {0};
    return buildPassiveFrame(0x02, dataBytes);
}

// 0x30 0x03: Set motion range — minAngle(float) + maxAngle(float)
QByteArray MotorUartProtocol::buildPassiveSetRange(float minAngle, float maxAngle)
{
    uint8_t dataBytes[8] = {0};
    union {
        float f;
        uint8_t b[4];
    } converter;

    converter.f = minAngle;
    for (int i = 0; i < 4; ++i) {
        dataBytes[i] = converter.b[i];
    }
    converter.f = maxAngle;
    for (int i = 0; i < 4; ++i) {
        dataBytes[i + 4] = converter.b[i];
    }

    return buildPassiveFrame(0x03, dataBytes);
}

// 0x30 0x04: Set pause time — uint32 milliseconds
QByteArray MotorUartProtocol::buildPassiveSetPause(uint32_t ms)
{
    uint8_t dataBytes[8] = {0};
    dataBytes[0] = static_cast<uint8_t>(ms & 0xFF);
    dataBytes[1] = static_cast<uint8_t>((ms >> 8) & 0xFF);
    dataBytes[2] = static_cast<uint8_t>((ms >> 16) & 0xFF);
    dataBytes[3] = static_cast<uint8_t>((ms >> 24) & 0xFF);

    return buildPassiveFrame(0x04, dataBytes);
}

// 0x30 0x05: Set direction — 1=flex优先 2=extend优先 3=alternate
QByteArray MotorUartProtocol::buildPassiveSetDirection(uint8_t dir)
{
    uint8_t dataBytes[8] = {0};
    dataBytes[0] = dir;
    return buildPassiveFrame(0x05, dataBytes);
}
// Parse data frame
bool MotorUartProtocol::parseRxFrame(const QByteArray &data, QVector<MotorStatus> &motorStatuses)
{
    QMutexLocker locker(&m_mutex); // Add mutex lock

    // Check frame length
    if (data.size() != 43) { // 2-byte header + 1-byte length + 39-byte data + 1-byte checksum
        emit parseError(QString("Invalid frame length: %1, expected 43").arg(data.size()));
        return false;
    }

    // Check frame header
    if (data[0] != '$' || data[1] != '#') {
        emit parseError("Invalid frame header");
        return false;
    }

    // Check length field
    if (static_cast<uint8_t>(data[2]) != 39) {
        emit parseError(QString("Invalid data length: %1, expected 39").arg(static_cast<uint8_t>(data[2])));
        return false;
    }

    // Checksum
    QByteArray payload = data.mid(3, 39);
    uint8_t checksum = calculateChecksum(payload);

    if (checksum != static_cast<uint8_t>(data[42])) {
        emit parseError(QString("Checksum error, calculated: %1, received: %2")
                            .arg(checksum).arg(static_cast<uint8_t>(data[42])));
        return false;
    }

    // Parse data
    for (int i = 0; i < 3; ++i) {
        int offset = i * 13;

        // Parse float
        union {
            float f;
            uint8_t b[4];
        } converter;

        // Position
        for (int j = 0; j < 4; ++j) {
            converter.b[j] = static_cast<uint8_t>(payload[offset + j]);
        }
        motorStatuses[i].position = converter.f;

        // Velocity
        for (int j = 0; j < 4; ++j) {
            converter.b[j] = static_cast<uint8_t>(payload[offset + 4 + j]);
        }
        motorStatuses[i].velocity = converter.f;

        // Temperature
        for (int j = 0; j < 4; ++j) {
            converter.b[j] = static_cast<uint8_t>(payload[offset + 8 + j]);
        }
        motorStatuses[i].temperature = converter.f;

        // Online status
        motorStatuses[i].online = static_cast<uint8_t>(payload[offset + 12]) != 0;
    }

    m_motorStatuses = motorStatuses;
    emit dataParsed(motorStatuses);

    return true;
}

QVector<MotorStatus> MotorUartProtocol::getMotorStatuses() const
{
    QMutexLocker locker(&m_mutex); // Add mutex lock
    return m_motorStatuses;
}

uint8_t MotorUartProtocol::calculateChecksum(const QByteArray &data)
{
    uint8_t sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += static_cast<uint8_t>(data[i]);
    }
    return 0xFF - sum;
}

// Helper function for float to byte conversion (STM32 format - little endian)
QByteArray MotorUartProtocol::floatToStm32Bytes(float value)
{
    QByteArray bytes;
    union {
        float f;
        uint8_t b[4];
    } converter;
    converter.f = value;
    
    // Little-endian order, low byte first
    for (int i = 0; i < 4; ++i) {
        bytes.append(converter.b[i]);
    }
    
    return bytes;
}

// Build motor control frame based on the provided protocol table
QByteArray MotorUartProtocol::buildMotorControlFrame(uint8_t cmd1, uint8_t cmd2, float floatValue)
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes
    frame.append(static_cast<char>(cmd1));
    frame.append(static_cast<char>(cmd2));
    
    // Data bytes (8 bytes - float value in STM32 format + 4 reserved bytes)
    QByteArray floatBytes = floatToStm32Bytes(floatValue);
    frame.append(floatBytes);
    // Add 4 more reserved data bytes (0x00)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build motor start/stop command
QByteArray MotorUartProtocol::buildMotorStartStopCommand(MotorAddress addr, bool start)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x01; // Start/stop command
    uint8_t data = start ? 0x01 : 0x00;
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x01; // Integer data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x11; // Integer data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes
    frame.append(static_cast<char>(cmd1));
    frame.append(static_cast<char>(cmd2));
    
    // Data bytes (8 bytes - integer data + 4 reserved bytes)
    frame.append(static_cast<char>(data));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    // Add 4 more reserved data bytes (0x00)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build motor angle command
QByteArray MotorUartProtocol::buildMotorAngleCommand(MotorAddress addr, float angle)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x01; // Angle command
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x06; // Float data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x16; // Float data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    return buildMotorControlFrame(cmd1, cmd2, angle);
}

// Build motor velocity command
QByteArray MotorUartProtocol::buildMotorVelocityCommand(MotorAddress addr, float velocity)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x02; // Velocity command
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x06; // Float data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x16; // Float data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    return buildMotorControlFrame(cmd1, cmd2, velocity);
}

// Build motor current command
QByteArray MotorUartProtocol::buildMotorCurrentCommand(MotorAddress addr, float current)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x03; // Current command
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x06; // Float data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x16; // Float data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    return buildMotorControlFrame(cmd1, cmd2, current);
}

// Build motor KP command
QByteArray MotorUartProtocol::buildMotorKPCommand(MotorAddress addr, float kp)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x04; // KP command
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x06; // Float data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x16; // Float data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    return buildMotorControlFrame(cmd1, cmd2, kp);
}

// Build motor KI command
QByteArray MotorUartProtocol::buildMotorKICommand(MotorAddress addr, float ki)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x05; // KI command
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x06; // Float data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x16; // Float data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    return buildMotorControlFrame(cmd1, cmd2, ki);
}

// Build motor B0 command
QByteArray MotorUartProtocol::buildMotorB0Command(MotorAddress addr, float b0)
{
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0x06; // B0 command
    
    // Set cmd1 based on motor address
    if (addr == MotorAddress::MOTOR_1) {
        cmd1 = 0x06; // Float data for motor 1
    } else if (addr == MotorAddress::MOTOR_2) {
        cmd1 = 0x16; // Float data for motor 2
    } else {
        // Unsupported motor address
        return QByteArray();
    }
    
    return buildMotorControlFrame(cmd1, cmd2, b0);
}

// Build MIT mode selection command for motor 2
QByteArray MotorUartProtocol::buildMotorMITModeCommand(uint8_t mode)
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes for motor 2 MIT mode selection
    frame.append(static_cast<char>(0x11)); // Integer data for motor 2
    frame.append(static_cast<char>(0x02)); // MIT mode selection command
    
    // Data bytes (mode: 0-MIT, 1-position, 2-velocity + 4 reserved bytes)
    frame.append(static_cast<char>(mode));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    // Add 4 more reserved data bytes (0x00)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build zero point setting command for motor 2
QByteArray MotorUartProtocol::buildMotorZeroSetCommand()
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes for motor 2 zero point setting
    frame.append(static_cast<char>(0x11)); // Integer data for motor 2
    frame.append(static_cast<char>(0x03)); // Zero point setting command
    
    // Data bytes (byte[0]=1,others=0, 8 bytes total)
    frame.append(static_cast<char>(0x01));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    // Add 4 more reserved data bytes (0x00)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build clear error command for motor 2
QByteArray MotorUartProtocol::buildMotorClearErrorCommand()
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes for motor 2 clear error
    frame.append(static_cast<char>(0x11)); // Integer data for motor 2
    frame.append(static_cast<char>(0x04)); // Clear error command
    
    // Data bytes (all zeros, 8 bytes total)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    // Add 4 more reserved data bytes (0x00)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build MIT dynamic parameters command for motor 2
QByteArray MotorUartProtocol::buildMotorMITDynamicParamsCommand(float targetAngle, float targetVelocity, float kp, float kd, float targetTorque)
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes for motor 2 MIT dynamic parameters
    frame.append(static_cast<char>(0x16)); // Float data for motor 2
    frame.append(static_cast<char>(0x01)); // MIT parameters setting command
    
    // Data bytes (8 bytes total - Byte0 to Byte7)
    
    // Byte0-Byte1: Target angle (16-bit, -12.57rad ~ 12.57rad)
    // Convert float angle to 16-bit unsigned integer
    // Range: -12.57rad to 12.57rad, mapping to 0 to 65535
    // Big-endian format (high byte first)
    float normalizedAngle = (targetAngle + 12.57f) / 25.14f; // Normalize to 0-1 range
    uint16_t angleInt = static_cast<uint16_t>(normalizedAngle * 65535.0f); // Map to 0-65535
    frame.append(static_cast<char>((angleInt >> 8) & 0xFF)); // Byte0: high byte (big-endian)
    frame.append(static_cast<char>(angleInt & 0xFF)); // Byte1: low byte (big-endian)
    
    // Byte2: Target velocity high 8 bits
    // Byte3[7-4]: Target velocity low 4 bits
    // Target velocity range: -33rad/s to 33rad/s, mapping to 0 to 4095
    float normalizedVelocity = (targetVelocity + 33.0f) / 66.0f; // Normalize to 0-1 range
    uint16_t velocityInt = static_cast<uint16_t>(normalizedVelocity * 4095.0f); // Map to 0-4095
    uint8_t velocityHigh8 = (velocityInt >> 4) & 0xFF; // Byte2: high 8 bits
    uint8_t velocityLow4 = velocityInt & 0x0F; // Byte3 high 4 bits
    frame.append(static_cast<char>(velocityHigh8)); // Byte2
    
    // Byte3[3-0]: kp high 4 bits
    // Byte4: kp low 8 bits
    // kp range: 0-500, mapping to 0-4095
    uint16_t kpInt = static_cast<uint16_t>((kp / 500.0f) * 4095.0f);
    uint8_t kpHigh4 = (kpInt >> 8) & 0x0F; // Byte3 low 4 bits
    uint8_t kpLow8 = kpInt & 0xFF; // Byte4
    uint8_t byte3 = ((velocityLow4 << 4) | kpHigh4); // Combine velocity low 4 bits and kp high 4 bits
    frame.append(static_cast<char>(byte3)); // Byte3
    frame.append(static_cast<char>(kpLow8)); // Byte4
    
    // Byte5: kd high 8 bits
    // Byte6[7-4]: kd low 4 bits
    // kd range: 0-5, mapping to 0-4095
    uint16_t kdInt = static_cast<uint16_t>((kd / 5.0f) * 4095.0f);
    uint8_t kdHigh8 = (kdInt >> 4) & 0xFF; // Byte5
    uint8_t kdLow4 = kdInt & 0x0F; // Byte6 high 4 bits
    frame.append(static_cast<char>(kdHigh8)); // Byte5
    
    // Byte6[3-0]: Target torque high 4 bits
    // Byte7: Target torque low 8 bits
    // Target torque range: -14N.m to 14N.m, mapping to 0-4095
    // 将-14到14的范围映射到0到4095
    float normalizedTorque = (targetTorque + 14.0f) / 28.0f; // 归一化到0-1范围
    uint16_t torqueInt = static_cast<uint16_t>(normalizedTorque * 4095.0f); // 映射到0-4095
    uint8_t torqueHigh4 = (torqueInt >> 8) & 0x0F; // Byte6 low 4 bits
    uint8_t torqueLow8 = torqueInt & 0xFF; // Byte7
    uint8_t byte6 = ((kdLow4 << 4) | torqueHigh4); // Combine kd low 4 bits and torque high 4 bits
    frame.append(static_cast<char>(byte6)); // Byte6
    frame.append(static_cast<char>(torqueLow8)); // Byte7
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build position mode control command for motor 2
QByteArray MotorUartProtocol::buildMotorPositionModeCommand(float targetAngle, float targetVelocity)
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes for motor 2 position mode control
    frame.append(static_cast<char>(0x16)); // Float data for motor 2
    frame.append(static_cast<char>(0x02)); // Position mode control command
    
    // Data bytes (8 bytes total - Byte0 to Byte7)
    
    // Byte0-Byte3: Target angle (float, from Motor2_para1_doubleSpinBox) - Little-endian format
    QByteArray angleBytes = floatToStm32Bytes(targetAngle);
    frame.append(angleBytes);
    
    // Byte4-Byte7: Target velocity (float, from Motor2_para2_doubleSpinBox) - Little-endian format
    QByteArray velocityBytes = floatToStm32Bytes(targetVelocity);
    frame.append(velocityBytes);
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}

// Build velocity mode control command for motor 2
QByteArray MotorUartProtocol::buildMotorVelocityModeCommand(float targetVelocity, float currentLimit)
{
    QByteArray frame;
    
    // Frame header (0x6D, 0x6A - 'm', 'j')
    frame.append(static_cast<char>(0x6D));
    frame.append(static_cast<char>(0x6A));
    
    // Address code (default: 0x01)
    frame.append(static_cast<char>(0x01));
    
    // Command codes for motor 2 velocity mode control
    frame.append(static_cast<char>(0x16)); // Float data for motor 2
    frame.append(static_cast<char>(0x03)); // Velocity mode control command
    
    // Data bytes (8 bytes total - Byte0 to Byte7)
    
    // Byte0-Byte3: Target velocity (float, from Motor2_para2_doubleSpinBox) - Little-endian format
    QByteArray velocityBytes = floatToStm32Bytes(targetVelocity);
    frame.append(velocityBytes);
    
    // Byte4-Byte7: Current limit (float, from Motor2_para6_doubleSpinBox) - Little-endian format
    QByteArray currentLimitBytes = floatToStm32Bytes(currentLimit);
    frame.append(currentLimitBytes);
    
    // Calculate checksum (sum of bytes 2-12)
    uint8_t checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum += static_cast<uint8_t>(frame[i]);
    }
    frame.append(static_cast<char>(checksum));
    
    return frame;
}