#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QThread>
#include <QVector>
#include "protocolhandler.h"
#include "MotorUartProtocol.h"

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);

    ~SerialWorker();

    void closePort();
    bool isConnected() const;
    void stop();

public slots:
    void init();  // Add init() method declaration
    void process();
    void writeData(const QByteArray &data);
    void setProtocolEnabled(bool enabled);

    void setDeviceId(uint8_t deviceId);
    void sendIOData(uint8_t deviceId, const QVector<ProtocolHandler::IOData> &ioData);
    void sendFloatData(uint8_t deviceId, const QVector<ProtocolHandler::FloatData> &floatData);

    void sendMotorCommand(MotorAddress addr, const QString &command);
    void sendMotorParameter(MotorAddress addr, int paramType, double value);
    
    // YAW, ROLL, PITCH axis parameter commands
    void sendYawParameter(int index, double value);
    void sendRollParameter(int index, double value);
    void sendPitchParameter(int index, double value);
    void sendFlashUpdate();
    
    // Motor 2 specific commands
    void sendMotorMITModeCommand(uint8_t mode);
    void sendMotorZeroSetCommand();
    void sendMotorClearErrorCommand();
    void sendMotorMITDynamicParamsCommand(float targetAngle, float targetVelocity, float kp, float kd, float targetTorque);
    // Position mode control command
    void sendMotorPositionModeCommand(float targetAngle, float targetVelocity);
    // Velocity mode control command
    void sendMotorVelocityModeCommand(float targetVelocity, float currentLimit);
    
    // Gimbal control commands
    void sendPitchUpCommand(uint8_t speed);
    void sendPitchDownCommand(uint8_t speed);
    void sendYawLeftCommand(uint8_t speed);
    void sendYawRightCommand(uint8_t speed);
    void sendPitchAngleCommand(int16_t angle);
    void sendYawAngleCommand(int16_t angle);
    void sendResetCommand();
    void sendLookDownCommand();
    void sendYawFollowCommand();
    void sendYawLockCommand();
    void sendStopCommand();

    bool openPort(const QString &portName, int baudRate,
                  QSerialPort::DataBits dataBits,
                  QSerialPort::StopBits stopBits,
                  QSerialPort::Parity parity);

signals:
    void dataReceived(const QByteArray &data);
    void motorStatusUpdated(const QVector<MotorStatus> &statuses);
    void protocolMessage(const QString &message);
    void errorOccurred(QSerialPort::SerialPortError error);

    void portOpened(bool success);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    bool m_stopRequested;

    QSerialPort *m_serial;
    ProtocolHandler *m_protocolHandler;
    MotorUartProtocol *m_motorProtocol;
    bool m_protocolEnabled;
    QByteArray m_rxBuffer;
};

#endif // SERIALWORKER_H