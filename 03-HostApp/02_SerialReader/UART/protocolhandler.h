#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QMutex>

class ProtocolHandler : public QObject
{
    Q_OBJECT

public:
    // Command type definitions
    enum CommandType {
        CMD_IO_DATA = 0x01,     // IO data
        CMD_FLOAT_DATA = 0x02,  // Float data
        CMD_ACK = 0x80          // Acknowledge response
    };

    // IO data structure
    struct IOData {
        uint8_t io_id;          // IO number (0-255)
        bool state;             // IO state (true/false)
    };

    // Float data structure
    struct FloatData {
        uint8_t sensor_id;      // Sensor number (0-255)
        float value;            // Float value
    };

    // Received frame data structure
    struct ProtocolFrame {
        uint8_t deviceId;
        CommandType command;
        QByteArray payload;
    };

    explicit ProtocolHandler(QObject *parent = nullptr);

    // Set local device ID
    void setDeviceId(uint8_t id);
    uint8_t getDeviceId() const;

    // Packing methods
    QByteArray packIOData(uint8_t deviceId, const QVector<IOData> &ioData);
    QByteArray packFloatData(uint8_t deviceId, const QVector<FloatData> &floatData);
    QByteArray packAck(uint8_t deviceId);

    // Process received data
    void processReceivedData(const QByteArray &data);

    // Clear receive buffer
    void clearBuffer();

signals:
    // Frame received signal
    void frameReceived(const ProtocolFrame &frame);

    // Error signal
    void protocolError(const QString &error);

private:
    mutable QMutex m_mutex; // Mutex for thread safety

    // Frame header definition
    static const char FRAME_HEADER = 0xAA;

    // Calculate checksum
    uint8_t calculateChecksum(const QByteArray &data);

    // Parse data frame
    bool parseFrame(const QByteArray &frame, ProtocolFrame &parsedFrame);

    // Protocol parsing buffer
    QByteArray buffer;

    // Local device ID
    uint8_t deviceId = 1;
};

#endif // PROTOCOLHANDLER_H