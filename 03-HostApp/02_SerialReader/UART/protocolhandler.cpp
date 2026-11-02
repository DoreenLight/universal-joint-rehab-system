#include "protocolhandler.h"
#include <QDebug>

// 设置本机设备ID
void ProtocolHandler::setDeviceId(uint8_t id)
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁
    deviceId = id;
}

uint8_t ProtocolHandler::getDeviceId() const
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁
    return deviceId;
}

ProtocolHandler::ProtocolHandler(QObject *parent)
    : QObject(parent)
{
}

// 计算校验和 (简单累加和)
uint8_t ProtocolHandler::calculateChecksum(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁

    uint8_t sum = 0;
    for (char byte : data) {
        sum += static_cast<uint8_t>(byte);
    }
    return sum;
}

// 打包IO数据
QByteArray ProtocolHandler::packIOData(uint8_t deviceId, const QVector<IOData> &ioData)
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁

    QByteArray frame;
    frame.append(FRAME_HEADER);                 // 帧头
    frame.append(static_cast<char>(deviceId));   // 设备ID
    frame.append(static_cast<char>(CMD_IO_DATA));// 命令字

    // 计算数据长度 (每个IO数据占2字节)
    uint8_t dataLength = ioData.size() * 2;
    frame.append(static_cast<char>(dataLength)); // 数据长度

    // 添加IO数据
    for (const IOData &data : ioData) {
        frame.append(static_cast<char>(data.io_id)); // IO编号
        frame.append(data.state ? 0x01 : 0x00);      // IO状态
    }

    // 计算并添加校验和
    uint8_t checksum = calculateChecksum(frame);
    frame.append(static_cast<char>(checksum));

    return frame;
}

// 打包浮点数据
QByteArray ProtocolHandler::packFloatData(uint8_t deviceId, const QVector<FloatData> &floatData)
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁

    QByteArray frame;
    frame.append(FRAME_HEADER);                 // 帧头
    frame.append(static_cast<char>(deviceId));   // 设备ID
    frame.append(static_cast<char>(CMD_FLOAT_DATA));// 命令字

    // 计算数据长度 (每个浮点数据占5字节)
    uint8_t dataLength = floatData.size() * 5;
    frame.append(static_cast<char>(dataLength)); // 数据长度

    // 添加浮点数据
    for (const FloatData &data : floatData) {
        frame.append(static_cast<char>(data.sensor_id)); // 传感器编号

        // 将float转换为4字节
        float value = data.value;
        char *p = reinterpret_cast<char*>(&value);
        for (int i = 0; i < 4; i++) {
            frame.append(p[i]);
        }
    }

    // 计算并添加校验和
    uint8_t checksum = calculateChecksum(frame);
    frame.append(static_cast<char>(checksum));

    return frame;
}

// 打包ACK响应
QByteArray ProtocolHandler::packAck(uint8_t deviceId)
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁

    QByteArray frame;
    frame.append(FRAME_HEADER);                 // 帧头
    frame.append(static_cast<char>(deviceId));   // 设备ID
    frame.append(static_cast<char>(CMD_ACK));    // 命令字
    frame.append(static_cast<char>(0));          // 数据长度=0

    // 计算并添加校验和
    uint8_t checksum = calculateChecksum(frame);
    frame.append(static_cast<char>(checksum));

    return frame;
}

// 处理接收数据
void ProtocolHandler::processReceivedData(const QByteArray &data)
{
    //添加互斥锁
    QMutexLocker locker(&m_mutex);

    // 添加到协议缓冲区
    buffer.append(data);

    // 查找帧头位置
    int startIndex = 0;
    while ((startIndex = buffer.indexOf(FRAME_HEADER, startIndex)) != -1) {
        // 检查是否有足够的数据
        if (buffer.size() - startIndex < 5) {
            // 数据不足，等待更多数据
            break;
        }

        // 获取数据长度
        uint8_t dataLength = static_cast<uint8_t>(buffer[startIndex + 3]);
        int frameLength = 5 + dataLength; // 帧头+设备ID+命令字+长度+数据+校验和

        // 检查是否有完整帧
        if (buffer.size() - startIndex < frameLength) {
            // 数据不足，等待更多数据
            break;
        }

        // 提取完整帧
        QByteArray frame = buffer.mid(startIndex, frameLength);

        // 尝试解析帧
        ProtocolFrame parsedFrame;
        if (parseFrame(frame, parsedFrame)) {
            emit frameReceived(parsedFrame);
        } else {
            emit protocolError("Invalid protocol frame or checksum error");
        }

        // 移除已处理的数据
        buffer.remove(0, startIndex + frameLength);
        startIndex = 0; // 重置查找位置
    }

    // 移除无效数据（没有帧头的数据）
    int lastHeaderPos = buffer.lastIndexOf(FRAME_HEADER);
    if (lastHeaderPos > 0) {
        buffer.remove(0, lastHeaderPos);
    } else if (lastHeaderPos == -1 && !buffer.isEmpty()) {
        buffer.clear();
    }
}

// 解析数据帧
bool ProtocolHandler::parseFrame(const QByteArray &frame, ProtocolFrame &parsedFrame)
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁

    // 检查帧长度
    if (frame.size() < 5) { // 最小帧长度(帧头+设备ID+命令字+长度+校验和)
        return false;
    }

    // 检查帧头
    if (static_cast<uint8_t>(frame[0]) != FRAME_HEADER) {
        return false;
    }

    // 提取设备ID
    parsedFrame.deviceId = static_cast<uint8_t>(frame[1]);

    // 提取命令字
    parsedFrame.command = static_cast<CommandType>(static_cast<uint8_t>(frame[2]));

    // 提取数据长度
    uint8_t dataLength = static_cast<uint8_t>(frame[3]);

    // 检查帧长度
    if (frame.size() != (5 + dataLength)) { // 帧头(1)+设备ID(1)+命令字(1)+长度(1)+数据(N)+校验和(1)
        return false;
    }

    // 提取数据内容
    parsedFrame.payload = frame.mid(4, dataLength);

    // 计算校验和
    QByteArray frameWithoutChecksum = frame.left(frame.size() - 1);
    uint8_t calculatedChecksum = calculateChecksum(frameWithoutChecksum);
    uint8_t receivedChecksum = static_cast<uint8_t>(frame[frame.size() - 1]);

    // 校验和检查
    if (calculatedChecksum != receivedChecksum) {
        return false;
    }

    return true;
}

void ProtocolHandler::clearBuffer()
{
    QMutexLocker locker(&m_mutex); // 添加互斥锁
    buffer.clear();
}




