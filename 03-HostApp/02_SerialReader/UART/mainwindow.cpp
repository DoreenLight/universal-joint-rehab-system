#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QKeyEvent> // 必须包含这个头文件来识别回车键


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , autoSendTimer(new QTimer(this))
    , serialThread(new QThread(this))
    , serialWorker(new SerialWorker)
    , uiUpdateTimer(new QTimer(this))
    , isConnected(false)              // 添加 isConnected 初始化
    , autoSendActive(false)           // 添加 autoSendActive 初始化
    , motor1Running(false)            // 添加电机状态初始化
    , motor2Running(false)
    , motor3Running(false)
{
    ui->setupUi(this);

    // 初始化串口设置
    initPortSettings();
    refreshPortList();
    initDoubleSpinBoxRanges();

    // 将工作对象移动到线程
    serialWorker->moveToThread(serialThread);

    // 连接工作线程槽
    connect(this, &MainWindow::openSerialPort, serialWorker, &SerialWorker::openPort);
    connect(this, &MainWindow::closeSerialPort, serialWorker, &SerialWorker::closePort);
    connect(this, &MainWindow::writeSerialData, serialWorker, &SerialWorker::writeData);

    connect(this, &MainWindow::setProtocolEnabled, serialWorker, &SerialWorker::setProtocolEnabled);

    connect(this, &MainWindow::setDeviceId, serialWorker, &SerialWorker::setDeviceId);
    connect(this, &MainWindow::sendIOData, serialWorker, &SerialWorker::sendIOData);
    connect(this, &MainWindow::sendFloatData, serialWorker, &SerialWorker::sendFloatData);

    // 连接新的信号槽
    connect(this, &MainWindow::sendMotorCommand, serialWorker, &SerialWorker::sendMotorCommand);
    connect(this, &MainWindow::sendMotorParameter, serialWorker, &SerialWorker::sendMotorParameter);
    
    // 连接吊舱控制信号
    connect(this, &MainWindow::sendPitchUpCommand, serialWorker, &SerialWorker::sendPitchUpCommand);
    connect(this, &MainWindow::sendPitchDownCommand, serialWorker, &SerialWorker::sendPitchDownCommand);
    connect(this, &MainWindow::sendYawLeftCommand, serialWorker, &SerialWorker::sendYawLeftCommand);
    connect(this, &MainWindow::sendYawRightCommand, serialWorker, &SerialWorker::sendYawRightCommand);
    connect(this, &MainWindow::sendPitchAngleCommand, serialWorker, &SerialWorker::sendPitchAngleCommand);
    connect(this, &MainWindow::sendYawAngleCommand, serialWorker, &SerialWorker::sendYawAngleCommand);
    connect(this, &MainWindow::sendResetCommand, serialWorker, &SerialWorker::sendResetCommand);
    connect(this, &MainWindow::sendLookDownCommand, serialWorker, &SerialWorker::sendLookDownCommand);
    connect(this, &MainWindow::sendYawFollowCommand, serialWorker, &SerialWorker::sendYawFollowCommand);
    connect(this, &MainWindow::sendYawLockCommand, serialWorker, &SerialWorker::sendYawLockCommand);
    connect(this, &MainWindow::sendStopCommand, serialWorker, &SerialWorker::sendStopCommand);
    
    // 连接YAW、ROLL、PITCH轴参数信号
    connect(this, &MainWindow::sendYawParameter, serialWorker, &SerialWorker::sendYawParameter);
    connect(this, &MainWindow::sendRollParameter, serialWorker, &SerialWorker::sendRollParameter);
    connect(this, &MainWindow::sendPitchParameter, serialWorker, &SerialWorker::sendPitchParameter);
    connect(this, &MainWindow::sendFlashUpdateCommand, serialWorker, &SerialWorker::sendFlashUpdate);
    
    // Connect new motor 2 specific commands
    connect(this, &MainWindow::sendMotorMITModeCommand, serialWorker, &SerialWorker::sendMotorMITModeCommand);
    connect(this, &MainWindow::sendMotorZeroSetCommand, serialWorker, &SerialWorker::sendMotorZeroSetCommand);
    connect(this, &MainWindow::sendMotorClearErrorCommand, serialWorker, &SerialWorker::sendMotorClearErrorCommand);
    connect(this, &MainWindow::sendMotorMITDynamicParamsCommand, serialWorker, &SerialWorker::sendMotorMITDynamicParamsCommand);
    connect(this, &MainWindow::sendMotorPositionModeCommand, serialWorker, &SerialWorker::sendMotorPositionModeCommand);
    connect(this, &MainWindow::sendMotorVelocityModeCommand, serialWorker, &SerialWorker::sendMotorVelocityModeCommand);

    // ★ 被动康复信号连接
    connect(this, &MainWindow::sendPassiveStart, serialWorker, &SerialWorker::sendPassiveStart);
    connect(this, &MainWindow::sendPassiveStop, serialWorker, &SerialWorker::sendPassiveStop);
    connect(this, &MainWindow::sendPassiveSetRange, serialWorker, &SerialWorker::sendPassiveSetRange);
    connect(this, &MainWindow::sendPassiveSetPause, serialWorker, &SerialWorker::sendPassiveSetPause);
    connect(this, &MainWindow::sendPassiveSetDirection, serialWorker, &SerialWorker::sendPassiveSetDirection);
    connect(serialWorker, &SerialWorker::portOpened, this, &MainWindow::handlePortOpened);

    // 连接工作线程信号
    // 串口线程接收到数据之后，将数据更新到数组之中
    connect(serialWorker, &SerialWorker::dataReceived, this, &MainWindow::handleDataReceived);

    // 串口线程解析接收到的数据时候，如果判断是更新吊舱电机的参数，则发射这个信号
    connect(serialWorker, &SerialWorker::motorStatusUpdated, this, &MainWindow::handleMotorStatus);
    connect(serialWorker, &SerialWorker::protocolMessage, this, &MainWindow::logProtocolMessage);

    connect(serialWorker, &SerialWorker::errorOccurred, this, &MainWindow::handleSerialError);

    // 启动工作线程
    connect(serialThread, &QThread::started, serialWorker, &SerialWorker::init);

    connect(serialThread, &QThread::finished, serialWorker, &QObject::deleteLater);
    // 启动线程，同时在 serialWorker 中进入主线程，目前主线程中什么都不做
    serialThread->start();

    // 设置UI更新定时器 (100ms更新一次)
    uiUpdateTimer->setInterval(100);
    connect(uiUpdateTimer, &QTimer::timeout, this, &MainWindow::updateUI);
    uiUpdateTimer->start();

    // 连接信号槽

    connect(autoSendTimer, &QTimer::timeout, this, &MainWindow::sendData);

    // 连接 focusOutEvent 事件 - 使用事件过滤器实现
    ui->Motor1_para1_doubleSpinBox->installEventFilter(this);
    ui->Motor1_para2_doubleSpinBox->installEventFilter(this);
    ui->Motor1_para3_doubleSpinBox->installEventFilter(this);
    ui->Motor1_para4_doubleSpinBox->installEventFilter(this);
    ui->Motor1_para5_doubleSpinBox->installEventFilter(this);
    ui->Motor2_para1_doubleSpinBox->installEventFilter(this);
    ui->Motor2_para2_doubleSpinBox->installEventFilter(this);
    ui->Motor2_para3_doubleSpinBox->installEventFilter(this);
    ui->Motor2_para4_doubleSpinBox->installEventFilter(this);
    ui->Motor2_para5_doubleSpinBox->installEventFilter(this);

    // 在 MainWindow 的构造函数中
    connect(ui->chkEnableProtocol, &QCheckBox::checkStateChanged, this, &MainWindow::onProtocolCheckboxStateChanged);
    connect(ui->chk_receiveProtocol, &QCheckBox::checkStateChanged, this, &MainWindow::onReceiveProtocolCheckboxStateChanged);




    // 设置自动发送间隔默认值
    ui->txt_SendMs->setRange(10, 60000); // 10ms到60s
    ui->txt_SendMs->setValue(1000);      // 默认1秒

    // 禁用发送按钮直到连接
    ui->Btn_send->setEnabled(false);
    ui->chk_TimeSend->setEnabled(false);

    // 设置接收区只读
    ui->Edit_recv->setReadOnly(true);


    

    // 连接零点设置、清除错误和MIT参数设置按钮信号
    connect(ui->pushButton_ZeroSet, &QPushButton::clicked, this, &MainWindow::on_pushButton_ZeroSet_clicked);
    connect(ui->pushButton_error_clear, &QPushButton::clicked, this, &MainWindow::on_pushButton_error_clear_clicked);
    connect(ui->pushButton_MITSet, &QPushButton::clicked, this, &MainWindow::on_pushButton_MITSet_clicked);
    connect(ui->pushButton_weizhiSet, &QPushButton::clicked, this, &MainWindow::on_pushButton_weizhiSet_clicked);
    connect(ui->pushButton_suduset, &QPushButton::clicked, this, &MainWindow::on_pushButton_suduset_clicked);
    connect(ui->btn_SendControl, &QPushButton::clicked, this, &MainWindow::sendRS00ControlCommand);
    connect(autoSendTimer, &QTimer::timeout, this, &MainWindow::sendRS00ControlCommand);
    // 确保最后调用 show()
    show();
}

MainWindow::~MainWindow()
{
    // if (serial->isOpen()) {
    //     serial->close();
    // }

    // 停止线程
    serialThread->quit();
    serialThread->wait();
    delete ui;
}

// 初始化串口设置
void MainWindow::initPortSettings()
{
    // 波特率默认值
    ui->cBox_baundrate->setCurrentIndex(7); // 115200

    // 数据位默认值
    ui->cBox_databit->setCurrentIndex(3); // 8

    // 停止位默认值
    ui->cBox_stopbit->setCurrentIndex(0); // 1

    // 校验位默认值
    ui->cBox_checkbit->setCurrentIndex(0); // none
}

// 添加处理串口打开结果的槽函数
void MainWindow::handlePortOpened(bool success)
{
    if (success) {
        ui->Btn_Openserial->setText("关闭串口");

        // 启用发送相关控件
        ui->Btn_send->setEnabled(true);
        ui->chk_TimeSend->setEnabled(true);

        // 设置协议使能状态
        emit setProtocolEnabled(ui->chkEnableProtocol->isChecked());

        QMessageBox::information(this, "Serial Port Status", "Serial port opened successfully");
        isConnected = true;
    } else {
        QMessageBox::critical(this, "Serial Port Error", "Failed to open serial port");
        isConnected = false;
    }
}

// 刷新串口列表
void MainWindow::refreshPortList()
{
    ui->cBox_searil->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QString portInfo = info.portName();
        qDebug() << "Port:" << info.portName() << "Description:" << info.description() << "Manufacturer:" << info.manufacturer();
        if (!info.description().isEmpty()) {
            portInfo += " " + info.description();
        } else if (!info.manufacturer().isEmpty()) {
            portInfo += " " + info.manufacturer();
        } else {
            portInfo += " (无描述信息)";
        }
        ui->cBox_searil->addItem(portInfo);
    }
}


void MainWindow::initDoubleSpinBoxRanges()
{
    // 参数控件 - Motor1
    ui->Motor1_para1_doubleSpinBox->setMinimum(-1e20);
    ui->Motor1_para1_doubleSpinBox->setMaximum(1e20);

    ui->Motor1_para2_doubleSpinBox->setMinimum(-1e20);
    ui->Motor1_para2_doubleSpinBox->setMaximum(1e20);

    ui->Motor1_para3_doubleSpinBox->setMinimum(-1e20);
    ui->Motor1_para3_doubleSpinBox->setMaximum(1e20);

    ui->Motor1_para4_doubleSpinBox->setMinimum(-1e20);
    ui->Motor1_para4_doubleSpinBox->setMaximum(1e20);

    ui->Motor1_para5_doubleSpinBox->setMinimum(-1e20);
    ui->Motor1_para5_doubleSpinBox->setMaximum(1e20);

    // 参数控件 - Motor2
    ui->Motor2_para1_doubleSpinBox->setMinimum(-1e20);
    ui->Motor2_para1_doubleSpinBox->setMaximum(1e20);

    ui->Motor2_para2_doubleSpinBox->setMinimum(-1e20);
    ui->Motor2_para2_doubleSpinBox->setMaximum(1e20);

    ui->Motor2_para3_doubleSpinBox->setMinimum(-1e20);
    ui->Motor2_para3_doubleSpinBox->setMaximum(1e20);

    ui->Motor2_para4_doubleSpinBox->setMinimum(-1e20);
    ui->Motor2_para4_doubleSpinBox->setMaximum(1e20);

    ui->Motor2_para5_doubleSpinBox->setMinimum(-1e20);
    ui->Motor2_para5_doubleSpinBox->setMaximum(1e20);
    

    // ... 其他需要取消限制的DoubleSpinBox ...
}

// 打开/关闭串口按钮点击
void MainWindow::on_Btn_Openserial_clicked()
{
    if (serialWorker->isConnected()) {
        // 关闭串口
        emit closeSerialPort();
        ui->Btn_Openserial->setText("打开串口");

        // 禁用发送相关控件
        ui->Btn_send->setEnabled(false);
        ui->chk_TimeSend->setEnabled(false);

        // 停止自动发送
        stopAutoSend();
    } else {
        // 配置串口参数
        QString portText = ui->cBox_searil->currentText();
        QString portName = portText.split(" ").first(); // 提取端口号部分
        int baudRate = ui->cBox_baundrate->currentText().toInt();

        // 设置数据位
        QSerialPort::DataBits dataBits;
        switch (ui->cBox_databit->currentIndex()) {
        case 0: dataBits = QSerialPort::Data5; break;
        case 1: dataBits = QSerialPort::Data6; break;
        case 2: dataBits = QSerialPort::Data7; break;
        case 3: dataBits = QSerialPort::Data8; break;
        default: dataBits = QSerialPort::Data8; break;
        }

        // 设置停止位
        QSerialPort::StopBits stopBits;
        switch (ui->cBox_stopbit->currentIndex()) {
        case 0: stopBits = QSerialPort::OneStop; break;
        case 1: stopBits = QSerialPort::OneAndHalfStop; break;
        case 2: stopBits = QSerialPort::TwoStop; break;
        default: stopBits = QSerialPort::OneStop; break;
        }

        // 设置校验位
        QSerialPort::Parity parity;
        switch (ui->cBox_checkbit->currentIndex()) {
        case 0: parity = QSerialPort::NoParity; break;
        case 1: parity = QSerialPort::OddParity; break;
        case 2: parity = QSerialPort::EvenParity; break;
        default: parity = QSerialPort::NoParity; break;
        }


        // 通过信号发送串口参数到工作线程
        emit openSerialPort(portName, baudRate, dataBits, stopBits, parity);



    }
}

// 清空接收区
void MainWindow::on_Btn_clearreceive_clicked()
{
    ui->Edit_recv->clear();
}

// 清空发送区
void MainWindow::on_Btn_ClearSend_clicked()
{
    ui->Edit_send->clear();
}

// 发送按钮点击
void MainWindow::on_Btn_send_clicked()
{
    sendData();
}

// 自动发送状态改变
void MainWindow::on_chk_TimeSend_stateChanged(int state)
{
    if (state == Qt::Checked) {
        startAutoSend();
    } else {
        stopAutoSend();
    }
}

// 启动自动发送
void MainWindow::startAutoSend()
{
    if (!autoSendActive) {
        int interval = ui->txt_SendMs->value();
        autoSendTimer->start(interval);
        autoSendActive = true;
    }
}

// 停止自动发送
void MainWindow::stopAutoSend()
{
    if (autoSendActive) {
        autoSendTimer->stop();
        autoSendActive = false;
    }
}

// 修改电机状态处理函数
void MainWindow::handleMotorStatus(const QVector<MotorStatus> &statuses)
{
    // 保存状态，等待UI更新定时器处理
    latestMotorStatuses = statuses;
}

// 添加UI更新函数
void MainWindow::updateUI()
{
    // 处理原始数据显示
    if (!latestRawData.isEmpty() && ui->chkShowRawData->isChecked()) {
        QString displayData = formatDataForDisplay(latestRawData);

        // 添加时间戳
        if (ui->chk_re_time->isChecked()) {
            QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
            displayData.prepend(timestamp);
        }

        // 添加到接收区
        ui->Edit_recv->appendPlainText(displayData);

        // 自动换行处理
        if (ui->chk_rev_line->isChecked()) {
            QTextCursor cursor = ui->Edit_recv->textCursor();
            cursor.movePosition(QTextCursor::End);
            ui->Edit_recv->setTextCursor(cursor);
        }

        latestRawData.clear();
    }

    // 更新电机状态显示
    if (!latestMotorStatuses.isEmpty() && latestMotorStatuses.size() >= 2) {
        // 记录日志
        QString degreeSymbol = QChar(0x00B0);
        logMotorMessage(QString("电机状态: 1(%1%2) 2(%3%4)")
                               .arg(latestMotorStatuses[0].position)
                               .arg(degreeSymbol)
                               .arg(latestMotorStatuses[1].position)
                               .arg(degreeSymbol));

        latestMotorStatuses.clear();
    }
}

// 发送数据
void MainWindow::sendData()
{
    if (!serialWorker->isConnected()) return;

    QString data = ui->Edit_send->toPlainText();

    // 处理发送新行
    if (ui->chk_send_line->isChecked()) {
        data += "\n";
    }

    QByteArray byteArray;
    if (ui->chk_send_hex->isChecked()) {
        // HEX发送模式
        QString cleanData = data.remove(' ').remove('\n').remove('\r');

        if (cleanData.length() % 2 != 0) {
            QMessageBox::warning(this, "Format Error", "HEX data length must be even");
            return;
        }

        for (int i = 0; i < cleanData.length(); i += 2) {
            bool ok;
            QString hexByte = cleanData.mid(i, 2);
            char byte = static_cast<char>(hexByte.toInt(&ok, 16));
            if (!ok) {
                QMessageBox::warning(this, "Format Error", "Invalid HEX character: " + hexByte);
                return;
            }
            byteArray.append(byte);
        }
    } else {
        // 文本发送模式
        byteArray = data.toUtf8();
    }

    // 通过信号发送数据到工作线程
    emit writeSerialData(byteArray);
}

// 处理接收数据 0
void MainWindow::handleDataReceived(const QByteArray &receivedData)
{
    // 保存数据，等待UI更新定时器处理
    latestRawData.append(receivedData);
 }

// 格式化显示数据
QString MainWindow::formatDataForDisplay(const QByteArray &data)
{
    if (ui->chk_rev_hex->isChecked()) {
        // HEX显示模式
        QString hexString;
        for (char byte : data) {
            hexString += QString("%1 ").arg(static_cast<quint8>(byte), 2, 16, QLatin1Char('0')).toUpper();
        }
        return hexString.trimmed();
    } else {
        // 文本显示模式
        // 替换控制字符为可见形式
        QString text = QString::fromUtf8(data);
        text.replace('\r', "\\r");
        text.replace('\n', "\\n");
        text.replace('\t', "\\t");
        return text;
    }
}

// 处理串口错误
void MainWindow::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    QString errorMsg;
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        errorMsg = "Device not found";
        break;
    case QSerialPort::PermissionError:
        errorMsg = "Permission error";
        break;
    case QSerialPort::OpenError:
        errorMsg = "Device already open";
        break;
    case QSerialPort::NotOpenError:
        errorMsg = "Device not open";
        break;
    case QSerialPort::WriteError:
        errorMsg = "Write error";
        break;
    case QSerialPort::ReadError:
        errorMsg = "Read error";
        break;
    case QSerialPort::ResourceError:
        errorMsg = "Device removed";
        break;
    case QSerialPort::UnsupportedOperationError:
        errorMsg = "Unsupported operation";
        break;
    case QSerialPort::TimeoutError:
        errorMsg = "Operation timeout";
        break;
    default:
        errorMsg = "Unknown error";
        break;
    }

    QMessageBox::critical(this, "Serial Port Error", errorMsg);

    // 发生严重错误时关闭串口
    if (error == QSerialPort::ResourceError) {
        if (serialWorker->isConnected()) {
            emit closeSerialPort();
        }
        ui->Btn_Openserial->setText("打开串口");
        isConnected = false;
        ui->Btn_send->setEnabled(false);
        ui->chk_TimeSend->setEnabled(false);
        stopAutoSend();
    }
}

// 检测串口按钮点击
void MainWindow::on_Btn_SerialCheck_clicked()
{
    refreshPortList();
    QMessageBox::information(this, "Serial Port Detection", "Available serial ports refreshed");
}




// 发送IO数据
void MainWindow::on_btnSendIO_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 创建IO数据
    QVector<ProtocolHandler::IOData> ioData;
    ProtocolHandler::IOData data;
    data.io_id = static_cast<uint8_t>(ui->spinIOID->value());

    data.state = ui->chkIOState->isChecked();
    ioData.append(data);

    // 通过信号发送数据到工作线程，让工作线程中的协议处理器打包数据
    emit sendIOData(static_cast<uint8_t>(ui->spinDeviceID->value()), ioData);

    // 更新日志
    logProtocolMessage(QString("发送IO数据: ID=%1, 状态=%2")
                           .arg(data.io_id)
                           .arg(data.state ? "ON" : "OFF"));
}

// 发送浮点数据
void MainWindow::on_btnSendFloat_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 创建浮点数据
    QVector<ProtocolHandler::FloatData> floatData;
    ProtocolHandler::FloatData data;
    data.sensor_id = static_cast<uint8_t>(ui->spinSensorID->value());
    data.value = ui->spinFloatValue->value();
    floatData.append(data);

    // 通过信号发送数据到工作线程，让工作线程中的协议处理器打包数据
    emit sendFloatData(static_cast<uint8_t>(ui->spinDeviceID->value()), floatData);

    // 更新日志
    logProtocolMessage(QString("发送浮点数据: 传感器ID=%1, 值=%2")
                           .arg(data.sensor_id)
                           .arg(data.value));
}

// 设备ID改变
void MainWindow::on_spinDeviceID_valueChanged(int arg1)
{
//    protocolHandler->setDeviceId(static_cast<uint8_t>(arg1));
    // 通过信号设置工作线程中的协议处理器设备ID
    emit setDeviceId(static_cast<uint8_t>(arg1));

}

// 记录协议消息
void MainWindow::logProtocolMessage(const QString &message)
{
    QString logEntry = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
    logEntry += message;
    ui->listProtocolLog->addItem(logEntry);
    ui->listProtocolLog->scrollToBottom();
}

// 处理电机状态
void MainWindow::processMotorStatus(const QVector<MotorStatus> &motorStatuses)
{
    if (motorStatuses.size() >= 2) {
        // 记录日志
        QString degreeSymbol = QChar(0x00B0); // Unicode度符号
        logMotorMessage(QString("电机状态: 1(%1%2) 2(%3%4)")
                               .arg(motorStatuses[0].position)
                               .arg(degreeSymbol)
                               .arg(motorStatuses[1].position)
                               .arg(degreeSymbol));
    }
}

// 记录吊舱协议消息
void MainWindow::logMotorMessage(const QString &message)
{
    QString logEntry = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
    logEntry += "[腕关节] " + message;

    ui->listProtocolLog->addItem(logEntry);
    ui->listProtocolLog->scrollToBottom();
}

// 更新电机状态显示
void MainWindow::updateMotorStatusDisplay()
{
    // Update motor status display
}

// 电机1启停按钮点击
void MainWindow::on_Motor1_qiting_btn_clicked()
{
    qDebug() << "on_Motor1_qiting_btn_clicked called, isConnected:" << isConnected;
    
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送指令");
        return;
    }
    
    // 切换电机运行状态
    motor1Running = !motor1Running;
    
    // 发送启停命令到工作线程
    qDebug() << "Emitting sendMotorCommand for Motor 1, command:" << (motor1Running ? "start" : "stop");
    emit sendMotorCommand(MotorAddress::MOTOR_1, motor1Running ? "start" : "stop");
    
    // 更新按钮文本
    ui->Motor1_qiting_btn->setText(motor1Running ? "电机1停止" : "电机1启动");
    
    // 记录日志
    logMotorMessage(QString("电机1%1指令发送成功")
                           .arg(motor1Running ? "启动" : "停止"));
}

// 电机2启停按钮点击
void MainWindow::on_Motor2_qiting_btn_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送指令");
        return;
    }

    // 切换电机运行状态
    motor2Running = !motor2Running;

    // 发送启停命令到工作线程
    emit sendMotorCommand(MotorAddress::MOTOR_2, motor2Running ? "start" : "stop");

    // ★ 如果是停止命令，执行清零操作 ★
    if (!motor2Running) {
        // 1. 清空界面上的五个参数输入框
        ui->Motor2_para1_doubleSpinBox->setValue(0.0);
        ui->Motor2_para2_doubleSpinBox->setValue(0.0);
        ui->Motor2_para3_doubleSpinBox->setValue(0.0);
        ui->Motor2_para4_doubleSpinBox->setValue(0.0);
        ui->Motor2_para5_doubleSpinBox->setValue(0.0);

        // 延时50ms后发送全零运控指令，避免与停止帧冲突
        QTimer::singleShot(50, this, [this]() {
            serialWorker->sendMotorMITDynamicParamsCommand(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            logMotorMessage("已发送全零运控指令，内部目标参数清零");
        });
        logMotorMessage("电机2停止，已清零界面参数");
    }

    // 更新按钮文本
    ui->Motor2_qiting_btn->setText(motor2Running ? "电机2停止" : "电机2启动");

    // 记录日志
    logMotorMessage(QString("电机2%1指令发送成功")
                        .arg(motor2Running ? "启动" : "停止"));
}

// Empty implementations for deprecated DJ motor buttons
void MainWindow::on_DJ1_QiTing_Btn_clicked()
{
    // Not implemented for wrist joint rehabilitation device
}

void MainWindow::on_DJ2_QiTing_Btn_clicked()
{
    // Not implemented for wrist joint rehabilitation device
}

void MainWindow::on_DJ3_QiTing_Btn_clicked()
{
    // Not implemented for wrist joint rehabilitation device
}


// 事件过滤器：处理电机参数输入框的“回车发送”事件
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 1. 检查是否为【键盘按下】事件
    if (event->type() == QEvent::KeyPress) {
        // 将通用事件强转为键盘事件
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // 2. 检查按下的是否是【回车键】(包含大键盘的 Return 和小键盘的 Enter)
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {

            // 3. 检查是哪个输入框触发的回车
            if (obj == ui->Motor1_para1_doubleSpinBox) {
                handle_ParaSpinBox_editingFinished(MotorAddress::MOTOR_1, 1, ui->Motor1_para1_doubleSpinBox->value());
            } else if (obj == ui->Motor1_para2_doubleSpinBox) {
                handle_ParaSpinBox_editingFinished(MotorAddress::MOTOR_1, 2, ui->Motor1_para2_doubleSpinBox->value());
            } else if (obj == ui->Motor1_para3_doubleSpinBox) {
                handle_ParaSpinBox_editingFinished(MotorAddress::MOTOR_1, 3, ui->Motor1_para3_doubleSpinBox->value());
            } else if (obj == ui->Motor1_para4_doubleSpinBox) {
                handle_ParaSpinBox_editingFinished(MotorAddress::MOTOR_1, 4, ui->Motor1_para4_doubleSpinBox->value());
            } else if (obj == ui->Motor1_para5_doubleSpinBox) {
                handle_ParaSpinBox_editingFinished(MotorAddress::MOTOR_1, 5, ui->Motor1_para5_doubleSpinBox->value());
            }
            // ------- 从这里开始是 Motor 2 的参数框 -------
            else if (obj == ui->Motor2_para1_doubleSpinBox ||
                     obj == ui->Motor2_para2_doubleSpinBox ||
                     obj == ui->Motor2_para3_doubleSpinBox ||
                     obj == ui->Motor2_para4_doubleSpinBox ||
                     obj == ui->Motor2_para5_doubleSpinBox) {
                // 什么也不做，或者清除焦点
                QDoubleSpinBox *currentSpinBox = qobject_cast<QDoubleSpinBox*>(obj);
                if (currentSpinBox) currentSpinBox->clearFocus();
                return true;
            }

            // 对于其他输入框，清除焦点
            QDoubleSpinBox *currentSpinBox = qobject_cast<QDoubleSpinBox*>(obj);
            if (currentSpinBox) {
                currentSpinBox->clearFocus();
            }
            return true;
        }
    }

    // 继续处理其他不受影响的普通事件
    return QMainWindow::eventFilter(obj, event);
}

// 参数编辑完成处理函数
void MainWindow::handle_ParaSpinBox_editingFinished(MotorAddress addr, int paramType, double value)
{
    if (!isConnected) return;

    qDebug() << "handle_ParaSpinBox_editingFinished called: motor=" << static_cast<int>(addr)
             << "paramType=" << paramType << "value=" << value;

    // 通过信号发送参数到工作线程
    emit sendMotorParameter(addr, paramType, value);

    QString cmdName;
    switch (paramType) {
    case 0: cmdName = "角度"; break;
    case 1: cmdName = "参数1"; break;
    case 2: cmdName = "参数2"; break;
    case 3: cmdName = "参数3"; break;
    case 4: cmdName = "参数4"; break;
    case 5: cmdName = "参数5"; break;
    default: return;
    }

    logMotorMessage(QString("发送电机%1%2指令: %3")
                           .arg(static_cast<int>(addr))
                           .arg(cmdName)
                           .arg(value));
}

// mainwindow.cpp
void MainWindow::onProtocolCheckboxStateChanged(int state)
{
    // 你的实现代码 here
    if (serialWorker) {
        serialWorker->setProtocolEnabled(state == Qt::Checked);
    }
}

// 不再需要这个函数，因为已经取消了协议复选框
void MainWindow::onReceiveProtocolCheckboxStateChanged(int state)
{
    Q_UNUSED(state)
    // 空实现，避免编译器警告
}

// 俯仰向上按钮按下
void MainWindow::on_Pitch_Up_Btn_pressed()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 使用默认速度值
    uint8_t speed = 128; // 默认速度约为22.5°/s
    emit sendPitchUpCommand(speed);
    logMotorMessage(QString("发送俯仰向上命令，速度: %1°/s").arg(22.5));
}

// 俯仰向上按钮松开
void MainWindow::on_Pitch_Up_Btn_released()
{
    if (!serialWorker->isConnected()) {
        return;
    }

    emit sendStopCommand();
    logMotorMessage(QString("Send motion stop command"));
}

// 俯仰向下按钮按下
void MainWindow::on_Pitch_Down_Btn_pressed()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 使用默认速度值
    uint8_t speed = 128; // 默认速度约为22.5°/s
    emit sendPitchDownCommand(speed);
    logMotorMessage(QString("Send pitch down command, speed: %1°/s").arg(22.5));
}

// 俯仰向下按钮松开
void MainWindow::on_Pitch_Down_Btn_released()
{
    if (!serialWorker->isConnected()) {
        return;
    }

    emit sendStopCommand();
    logMotorMessage(QString("Send motion stop command"));
}

// 航向向左按钮按下
void MainWindow::on_Yaw_left_Btn_pressed()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 使用默认速度值
    uint8_t speed = 128; // 默认速度约为22.5°/s
    emit sendYawLeftCommand(speed);
    logMotorMessage(QString("Send yaw left command, speed: %1°/s").arg(22.5));
}

// 航向向左按钮松开
void MainWindow::on_Yaw_left_Btn_released()
{
    if (!serialWorker->isConnected()) {
        return;
    }

    emit sendStopCommand();
    logMotorMessage(QString("Send motion stop command"));
}

// 航向向右按钮按下
void MainWindow::on_Yaw_Right_Btn_pressed()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 使用默认速度值
    uint8_t speed = 128; // 默认速度约为22.5°/s
    emit sendYawRightCommand(speed);
    logMotorMessage(QString("Send yaw right command, speed: %1°/s").arg(22.5));
}

// 航向向右按钮松开
void MainWindow::on_Yaw_Right_Btn_released()
{
    if (!serialWorker->isConnected()) {
        return;
    }

    emit sendStopCommand();
    logMotorMessage(QString("Send motion stop command"));
}

// 航向角度设置按钮点击
void MainWindow::on_Yaw_AngleSet_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 使用默认角度值
    int16_t angle = 0; // 默认角度为0°

    emit sendYawAngleCommand(angle);
    logMotorMessage(QString("发送航向角度设置命令，角度: %1°").arg(angle));
}

// 俯仰角度设置按钮点击
void MainWindow::on_Pitch_AngleSet_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    // 使用默认角度值
    int16_t angle = 0; // 默认角度为0°

    emit sendPitchAngleCommand(angle);
    logMotorMessage(QString("发送俯仰角度设置命令，角度: %1°").arg(angle));
}

// 一键回中按钮点击
void MainWindow::on_Reset_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    emit sendResetCommand();
    logMotorMessage("Send reset command");
}

// 垂直俯视按钮点击
void MainWindow::on_LookDown_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    emit sendLookDownCommand();
    logMotorMessage("Send look down command");
}

// 航向跟随按钮点击
void MainWindow::on_Yaw_Follw_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    emit sendYawFollowCommand();
    logMotorMessage("Send yaw follow mode command");
}

// 航向锁定按钮点击
void MainWindow::on_Yaw_Lock_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }

    emit sendYawLockCommand();
    logMotorMessage("Send yaw lock mode command");
}

// FLASH更新按钮点击
void MainWindow::on_Wirte_Flash_Btn_clicked()
{
    if (!serialWorker->isConnected()) {
        QMessageBox::warning(this, "Warning", "Please open serial port first");
        return;
    }
    
    emit sendFlashUpdateCommand();
    logMotorMessage("Send FLASH update command");
}

// 更新模式选择按钮状态




// 处理零点设置按钮点击事件
void MainWindow::on_pushButton_ZeroSet_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送零点设置指令");
        return;
    }
    
    // 发送零点设置指令
    emit sendMotorZeroSetCommand();
    logMotorMessage("发送电机2零点设置指令成功");
}

// // 处理清除错误按钮点击事件
void MainWindow::on_pushButton_error_clear_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送清除错误指令");
        return;
    }
    
    // 发送清除错误指令
    emit sendMotorClearErrorCommand();
    logMotorMessage("发送电机2清除错误指令成功");
}

// 处理MIT参数设置按钮点击事件
void MainWindow::on_pushButton_MITSet_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送MIT参数设置指令");
        return;
    }
    
    // 从界面获取参数
    float targetVelocityFloat = ui->Motor2_para2_doubleSpinBox->value(); // 目标速度
    float targetAngleFloat = ui->Motor2_para1_doubleSpinBox->value(); // 目标角度
    float kpFloat = ui->Motor2_para4_doubleSpinBox->value(); // 刚度kp
    float kdFloat = ui->Motor2_para5_doubleSpinBox->value(); // 阻尼kd
    float targetTorqueFloat = ui->Motor2_para3_doubleSpinBox->value(); // 目标力矩
    
    // 发送MIT动态参数指令（参数转换在协议层处理）
    emit sendMotorMITDynamicParamsCommand(targetAngleFloat, targetVelocityFloat, kpFloat, kdFloat, targetTorqueFloat);
    logMotorMessage(QString("发送电机2MIT参数设置指令成功，目标速度: %1, 目标角度: %2, kp: %3, kd: %4, 目标力矩: %5 Nm").arg(targetVelocityFloat).arg(targetAngleFloat).arg(kpFloat).arg(kdFloat).arg(targetTorqueFloat));
}

// 处理位置设置按钮点击事件
void MainWindow::on_pushButton_weizhiSet_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送位置设置指令");
        return;
    }
    
    // 从界面获取参数
    float targetAngleFloat = ui->Motor2_para1_doubleSpinBox->value(); // 目标角度
    float targetVelocityFloat = ui->Motor2_para2_doubleSpinBox->value(); // 目标速度
    
    // 发送位置控制指令（参数转换在协议层处理）
    emit sendMotorPositionModeCommand(targetAngleFloat, targetVelocityFloat);
    logMotorMessage(QString("发送电机2位置控制指令成功，目标角度: %1, 目标速度: %2").arg(targetAngleFloat).arg(targetVelocityFloat));
}

// 处理速度设置按钮点击事件
void MainWindow::on_pushButton_suduset_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法发送速度设置指令");
        return;
    }
    
    // 从界面获取参数
    float targetVelocityFloat = ui->Motor2_para2_doubleSpinBox->value(); // 目标速度
    float currentLimitFloat = ui->Motor2_para6_doubleSpinBox->value(); // 电流限制
    
    // 发送速度控制指令（参数转换在协议层处理）
    emit sendMotorVelocityModeCommand(targetVelocityFloat, currentLimitFloat);
    logMotorMessage(QString("发送电机2速度控制指令成功，目标速度: %1, 电流限制: %2").arg(targetVelocityFloat).arg(currentLimitFloat));
}


void MainWindow::sendRS00ControlCommand()
{
    if (!serialWorker->isConnected()) return;

    // 从界面获取运控参数（使用电机2的五个输入框）
    float pos   = ui->Motor2_para1_doubleSpinBox->value();  // 目标位置 (rad)
    float vel   = ui->Motor2_para2_doubleSpinBox->value();  // 目标速度 (rad/s)
    float torque= ui->Motor2_para3_doubleSpinBox->value();  // 前馈力矩 (Nm)
    float kp    = ui->Motor2_para4_doubleSpinBox->value();  // Kp
    float kd    = ui->Motor2_para5_doubleSpinBox->value();  // Kd

    // 调用 SerialWorker 的运控指令发送函数
    serialWorker->sendMotorMITDynamicParamsCommand(pos, vel, kp, kd, torque);
}

// ====================================================================
// ★ 被动康复 — UI交互槽函数
// ====================================================================

void MainWindow::on_btnPassiveStart_clicked()
{
    if (!isConnected) {
        logMotorMessage("未连接串口，无法启动被动康复");
        return;
    }

    double speed = ui->spinPassiveSpeed->value();
    int cycles = ui->spinPassiveCycles->value();

    // 先发送参数设置
    float minDeg = static_cast<float>(ui->spinPassiveRangeMin->value());
    float maxDeg = static_cast<float>(ui->spinPassiveRangeMax->value());
    float minRad = minDeg * 3.14159265f / 180.0f;
    float maxRad = maxDeg * 3.14159265f / 180.0f;
    emit sendPassiveSetRange(minRad, maxRad);

    uint32_t pauseMs = static_cast<uint32_t>(ui->spinPassivePause->value());
    emit sendPassiveSetPause(pauseMs);

    uint8_t dir = static_cast<uint8_t>(ui->comboPassiveDir->currentIndex() + 1);
    emit sendPassiveSetDirection(dir);

    // 发送启动指令
    emit sendPassiveStart(static_cast<float>(speed), static_cast<uint32_t>(cycles));

    logMotorMessage(QString("被动康复启动: 速度=%1 rad/s, 循环=%2, 范围=[%3°, %4°]")
                        .arg(speed).arg(cycles).arg(minDeg).arg(maxDeg));
}

void MainWindow::on_btnPassiveStop_clicked()
{
    if (!isConnected) return;

    emit sendPassiveStop();
    logMotorMessage("被动康复停止");
}

void MainWindow::on_btnPassiveSetRange_clicked()
{
    if (!isConnected) return;

    float minDeg = static_cast<float>(ui->spinPassiveRangeMin->value());
    float maxDeg = static_cast<float>(ui->spinPassiveRangeMax->value());
    float minRad = minDeg * 3.14159265f / 180.0f;
    float maxRad = maxDeg * 3.14159265f / 180.0f;
    emit sendPassiveSetRange(minRad, maxRad);

    uint32_t pauseMs = static_cast<uint32_t>(ui->spinPassivePause->value());
    emit sendPassiveSetPause(pauseMs);

    uint8_t dir = static_cast<uint8_t>(ui->comboPassiveDir->currentIndex() + 1);
    emit sendPassiveSetDirection(dir);

    logMotorMessage(QString("被动康复参数应用: 范围=[%1°, %2°], 暂停=%3ms")
                        .arg(minDeg).arg(maxDeg).arg(pauseMs));
}

void MainWindow::on_spinPassiveSpeed_valueChanged(double value)
{
    Q_UNUSED(value);
    // 实时速度值变化时不做发送，等点击开始或应用参数时统一发送
}

void MainWindow::on_spinPassiveCycles_valueChanged(int value)
{
    Q_UNUSED(value);
}

void MainWindow::on_spinPassivePause_valueChanged(int value)
{
    Q_UNUSED(value);
}

void MainWindow::on_comboPassiveDir_currentIndexChanged(int index)
{
    Q_UNUSED(index);
}
