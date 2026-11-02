#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QThread>

// #include "protocolhandler.h"  // 包含协议处理器头文件
// #include "MotorUartProtocol.h"  // 包含吊舱协议头文件
#include "SerialWorker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 添加前向声明
class SerialWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    //   QSerialPort *serial;
    QTimer *autoSendTimer;

    // 添加工作线程相关成员
    QThread *serialThread;
    SerialWorker *serialWorker;

    // 添加UI更新定时器
    QTimer *uiUpdateTimer;
    QVector<MotorStatus> latestMotorStatuses;
    QByteArray latestRawData;

    QMap<QObject*, double> lastSentValues; // 记录每个输入框上一次发送的数值
private slots:
    // 串口操作
    void on_Btn_SerialCheck_clicked();
    void on_Btn_Openserial_clicked();

    // 接收操作
    void on_Btn_clearreceive_clicked();

    // 发送操作
    void on_Btn_send_clicked();
    void on_Btn_ClearSend_clicked();

    // 自动发送控制
    void on_chk_TimeSend_stateChanged(int state);



    // 串口数据处理
    void handleDataReceived(const QByteArray &receivedData);
    void handleSerialError(QSerialPort::SerialPortError error);

    // 协议操作
    void on_btnSendIO_clicked();
    void on_btnSendFloat_clicked();

    void handlePortOpened(bool success);

    // // 协议处理器信号
    // void handleProtocolFrame(const ProtocolHandler::ProtocolFrame &frame);
    // void handleProtocolError(const QString &error);

    // 设备ID改变
    void on_spinDeviceID_valueChanged(int arg1);

    // 吊舱控制相关槽函数
    void on_DJ1_QiTing_Btn_clicked();
    void on_DJ2_QiTing_Btn_clicked();
    void on_DJ3_QiTing_Btn_clicked();
    
    // 电机控制相关槽函数
    void on_Motor1_qiting_btn_clicked();
    void on_Motor2_qiting_btn_clicked();
    
    // 吊舱控制按钮
    void on_Pitch_Up_Btn_pressed();
    void on_Pitch_Up_Btn_released();
    void on_Pitch_Down_Btn_pressed();
    void on_Pitch_Down_Btn_released();
    void on_Yaw_left_Btn_pressed();
    void on_Yaw_left_Btn_released();
    void on_Yaw_Right_Btn_pressed();
    void on_Yaw_Right_Btn_released();
    void on_Yaw_AngleSet_Btn_clicked();
    void on_Pitch_AngleSet_Btn_clicked();
    void on_Reset_Btn_clicked();
    void on_LookDown_Btn_clicked();
    void on_Yaw_Follw_Btn_clicked();
    void on_Yaw_Lock_Btn_clicked();
    
    // FLASH更新按钮点击
    void on_Wirte_Flash_Btn_clicked();

    // 复选框状态改变
    void onProtocolCheckboxStateChanged(int state);
    void onReceiveProtocolCheckboxStateChanged(int state);
    

    
    // 处理零点设置、清除错误和MIT参数设置按钮点击事件
    void on_pushButton_ZeroSet_clicked();
    void on_pushButton_error_clear_clicked();
    void on_pushButton_MITSet_clicked();
    // 处理位置设置按钮点击事件
    void on_pushButton_weizhiSet_clicked();
    // 处理速度设置按钮点击事件
    void on_pushButton_suduset_clicked();


     // 统一处理函数：type=0表示角度，1-3表示参数1-3
    void handle_ParaSpinBox_editingFinished(MotorAddress addr, int paramType, double value);

    void handleMotorStatus(const QVector<MotorStatus> &statuses); // 添加声明
    void updateUI(); // 添加声明
    // ★ 被动康复UI交互
    void on_btnPassiveStart_clicked();
    void on_btnPassiveStop_clicked();
    void on_btnPassiveSetRange_clicked();
    void on_spinPassiveSpeed_valueChanged(double value);
    void on_spinPassiveCycles_valueChanged(int value);
    void on_spinPassivePause_valueChanged(int value);
    void on_comboPassiveDir_currentIndexChanged(int index);


private:
    void initPortSettings();
    void refreshPortList();
    void initDoubleSpinBoxRanges();

    void startAutoSend();
    void stopAutoSend();
    void sendData();
    QString formatDataForDisplay(const QByteArray &data);
    


    void updateMotorStatusDisplay();



    bool isConnected = false;
    bool autoSendActive = false;

    // 电机控制相关变量
    bool motor1Running = false;
    bool motor2Running = false;
    bool motor3Running = false;

    // 处理电机协议数据
    void handleMotorData(const QByteArray &data);
    void processMotorStatus(const QVector<MotorStatus> &motorStatuses);

    //串口通信协议处理相关
    void logProtocolMessage(const QString &message);

    // // 处理协议帧
    // void processIOData(const ProtocolHandler::ProtocolFrame &frame);
    // void processFloatData(const ProtocolHandler::ProtocolFrame &frame);
    // void processAck(const ProtocolHandler::ProtocolFrame &frame);

    // // 添加这些参数发送函数
    // void sendParameter1(MotorAddress addr, float value);
    // void sendParameter2(MotorAddress addr, float value);
    // void sendParameter3(MotorAddress addr, float value);

    // ... 其他成员函数 ...
    void logMotorMessage(const QString &message);

     void sendRS00ControlCommand();   // 发送运控指令

    // 添加信号声明
signals:
    void openSerialPort(const QString &portName, int baudRate,
                        QSerialPort::DataBits dataBits,
                        QSerialPort::StopBits stopBits,
                        QSerialPort::Parity parity);
    void closeSerialPort();
    void writeSerialData(const QByteArray &data);

// 协议控制相关信号
    void setProtocolEnabled(bool enabled);
    void setDeviceId(uint8_t deviceId);

    void sendIOData(uint8_t deviceId, const QVector<ProtocolHandler::IOData> &ioData);
    void sendFloatData(uint8_t deviceId, const QVector<ProtocolHandler::FloatData> &floatData);

    void sendMotorCommand(MotorAddress addr, const QString &command);
    void sendMotorParameter(MotorAddress addr, int paramType, double value);
    
    // 吊舱控制信号
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
    
    // YAW、ROLL、PITCH轴参数信号
    void sendYawParameter(int index, double value);
    void sendRollParameter(int index, double value);
    void sendPitchParameter(int index, double value);
    void sendFlashUpdateCommand();
    
    // Motor 2 specific signals
    void sendMotorMITModeCommand(uint8_t mode);
    void sendMotorZeroSetCommand();
    void sendMotorClearErrorCommand();
    void sendMotorMITDynamicParamsCommand(float targetAngle, float targetVelocity, float kp, float kd, float targetTorque);
    // Position mode control signal
    void sendMotorPositionModeCommand(float targetAngle, float targetVelocity);
    // Velocity mode control signal
    void sendMotorVelocityModeCommand(float targetVelocity, float currentLimit);
    // ★ 被动康复控制信号
    void sendPassiveStart(float speed, uint32_t cycles);
    void sendPassiveStop();
    void sendPassiveSetRange(float minAngle, float maxAngle);
    void sendPassiveSetPause(uint32_t ms);
    void sendPassiveSetDirection(uint8_t dir);

};
#endif // MAINWINDOW_H
