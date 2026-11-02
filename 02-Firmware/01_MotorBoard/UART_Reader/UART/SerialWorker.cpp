#include "SerialWorker.h"
#include <QDebug>
#include <QThread>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
    , m_serial(nullptr)
    , m_protocolHandler(nullptr)
    , m_motorProtocol(nullptr)
    , m_protocolEnabled(false)
    , m_stopRequested(false)
{
}

SerialWorker::~SerialWorker()
{
    closePort();
    delete m_serial;
    delete m_protocolHandler;
    delete m_motorProtocol;
}

void SerialWorker::init()
{
    m_serial = new QSerialPort();
    m_protocolHandler = new ProtocolHandler();
    m_motorProtocol = new MotorUartProtocol();

    connect(m_serial, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialWorker::handleError);
}

void SerialWorker::process()
{
    while (!m_stopRequested) {
        QThread::msleep(100);
    }
}

void SerialWorker::stop()
{
    m_stopRequested = true;
}

void SerialWorker::closePort()
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
    }
}

void SerialWorker::handleReadyRead()
{
    QByteArray data = m_serial->readAll();
    emit dataReceived(data);
}

void SerialWorker::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    emit errorOccurred(m_serial->error());
}

void SerialWorker::writeData(const QByteArray &data)
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->write(data);
        m_serial->flush();
    }
}

void SerialWorker::setProtocolEnabled(bool enabled)
{
    m_protocolEnabled = enabled;
}

void SerialWorker::setDeviceId(uint8_t deviceId)
{
    if (m_protocolHandler) {
        m_protocolHandler->setDeviceId(deviceId);
    }
}

void SerialWorker::sendIOData(uint8_t deviceId, const QVector<ProtocolHandler::IOData> &ioData)
{
    if (m_protocolHandler && m_protocolEnabled) {
        QByteArray data = m_protocolHandler->packIOData(deviceId, ioData);
        writeData(data);
    }
}

void SerialWorker::sendFloatData(uint8_t deviceId, const QVector<ProtocolHandler::FloatData> &floatData)
{
    if (m_protocolHandler && m_protocolEnabled) {
        QByteArray data = m_protocolHandler->packFloatData(deviceId, floatData);
        writeData(data);
    }
}

void SerialWorker::sendMotorCommand(MotorAddress addr, const QString &command)
{
    qDebug() << "SerialWorker::sendMotorCommand called, motor:" << static_cast<int>(addr) << "command:" << command;
    qDebug() << "m_motorProtocol:" << m_motorProtocol;
    
    if (m_motorProtocol) {
        QByteArray data;
        if (command == "start") {
            data = m_motorProtocol->buildMotorStartStopCommand(addr, true);
        } else if (command == "stop") {
            data = m_motorProtocol->buildMotorStartStopCommand(addr, false);
        }
        
        qDebug() << "Built command frame:" << data.toHex();
        
        if (!data.isEmpty()) {
            writeData(data);
        }
    }
}

void SerialWorker::sendMotorParameter(MotorAddress addr, int paramType, double value)
{
    if (m_motorProtocol) {
        QByteArray data;
        switch (paramType) {
        case 1: // 参数1 - 角度指令
            data = m_motorProtocol->buildMotorAngleCommand(addr, static_cast<float>(value));
            break;
        case 2: // 参数2 - 速度指令
            data = m_motorProtocol->buildMotorVelocityCommand(addr, static_cast<float>(value));
            break;
        case 3: // 参数3 - 电流指令
            data = m_motorProtocol->buildMotorCurrentCommand(addr, static_cast<float>(value));
            break;
        case 4: // 参数4 - KP
            data = m_motorProtocol->buildMotorKPCommand(addr, static_cast<float>(value));
            break;
        case 5: // 参数5 - KI
            data = m_motorProtocol->buildMotorKICommand(addr, static_cast<float>(value));
            break;
        default:
            return;
        }
        if (!data.isEmpty()) {
            writeData(data);
        }
    }
}

void SerialWorker::sendYawParameter(int index, double value)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildYawParaCommand(static_cast<uint8_t>(index), static_cast<float>(value));
        writeData(data);
    }
}

void SerialWorker::sendRollParameter(int index, double value)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildRollParaCommand(static_cast<uint8_t>(index), static_cast<float>(value));
        writeData(data);
    }
}

void SerialWorker::sendPitchParameter(int index, double value)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildPitchParaCommand(static_cast<uint8_t>(index), static_cast<float>(value));
        writeData(data);
    }
}

// Empty implementations for deprecated functions to avoid linker errors
void SerialWorker::sendFlashUpdate()
{
    // Not implemented for wrist joint rehabilitation device
}

// Motor 2 specific commands
void SerialWorker::sendMotorMITModeCommand(uint8_t mode)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildMotorMITModeCommand(mode);
        writeData(data);
    }
}

void SerialWorker::sendMotorZeroSetCommand()
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildMotorZeroSetCommand();
        writeData(data);
    }
}

void SerialWorker::sendMotorClearErrorCommand()
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildMotorClearErrorCommand();
        writeData(data);
    }
}

void SerialWorker::sendMotorMITDynamicParamsCommand(float targetAngle, float targetVelocity, float kp, float kd, float targetTorque)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildMotorMITDynamicParamsCommand(targetAngle, targetVelocity, kp, kd, targetTorque);
        // ===================== 新增打印代码 =====================
        qDebug() << "======================================================";
        qDebug() << "发送 MIT 动态参数指令";
        qDebug() << "输入值 -> 角度:" << targetAngle << "速度:" << targetVelocity << "kp:" << kp << "kd:" << kd << "扭矩:" << targetTorque;
        qDebug() << "发送的完整帧(16进制):" << data.toHex(' '); // 空格分隔每字节
        qDebug() << "======================================================\n";
        // ========================================================
        writeData(data);
    }
}

void SerialWorker::sendMotorPositionModeCommand(float targetAngle, float targetVelocity)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildMotorPositionModeCommand(targetAngle, targetVelocity);
        // ===================== 打印调试信息 =====================
        qDebug() << "======================================================";
        qDebug() << "发送位置控制指令";
        qDebug() << "输入值 -> 目标角度:" << targetAngle << "目标速度:" << targetVelocity;
        qDebug() << "发送的完整帧(16进制):" << data.toHex(' '); // 空格分隔每字节
        qDebug() << "======================================================\n";
        // ========================================================
        writeData(data);
    }
}

void SerialWorker::sendMotorVelocityModeCommand(float targetVelocity, float currentLimit)
{
    if (m_motorProtocol) {
        QByteArray data = m_motorProtocol->buildMotorVelocityModeCommand(targetVelocity, currentLimit);
        // ===================== 打印调试信息 =====================
        qDebug() << "======================================================";
        qDebug() << "发送速度控制指令";
        qDebug() << "输入值 -> 目标速度:" << targetVelocity << "电流限制:" << currentLimit;
        qDebug() << "发送的完整帧(16进制):" << data.toHex(' '); // 空格分隔每字节
        qDebug() << "======================================================\n";
        // ========================================================
        writeData(data);
    }
}

void SerialWorker::sendPitchUpCommand(uint8_t speed)
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendPitchDownCommand(uint8_t speed)
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendYawLeftCommand(uint8_t speed)
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendYawRightCommand(uint8_t speed)
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendPitchAngleCommand(int16_t angle)
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendYawAngleCommand(int16_t angle)
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendResetCommand()
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendLookDownCommand()
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendYawFollowCommand()
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendYawLockCommand()
{
    // Not implemented for wrist joint rehabilitation device
}

void SerialWorker::sendStopCommand()
{
    // Not implemented for wrist joint rehabilitation device
}

bool SerialWorker::openPort(const QString &portName, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits, QSerialPort::Parity parity)
{
    if (m_serial) {
        m_serial->setPortName(portName);
        m_serial->setBaudRate(baudRate);
        m_serial->setDataBits(dataBits);
        m_serial->setStopBits(stopBits);
        m_serial->setParity(parity);
        
        if (m_serial->open(QIODevice::ReadWrite)) {
            emit portOpened(true);
            return true;
        } else {
            emit portOpened(false);
            return false;
        }
    }
    emit portOpened(false);
    return false;
}

bool SerialWorker::isConnected() const
{
    return m_serial && m_serial->isOpen();
}