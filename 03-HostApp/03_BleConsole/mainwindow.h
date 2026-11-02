#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyService>
#include <QMainWindow>
#include <QQueue>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QBluetoothDeviceDiscoveryAgent;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLowEnergyController;
class QLowEnergyService;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUiDefaults();
    void setupConnections();
    void startDeviceSearch();
    void addDiscoveredDevice(const QBluetoothDeviceInfo &deviceInfo);
    void connectToSelectedDevice();
    void connectToDevice(const QBluetoothDeviceInfo &deviceInfo);
    void resetBleConnection();
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceScanDone();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onBleNotification(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    QByteArray buildIntFrame(quint8 cmd1, quint8 cmd2, qint32 value) const;
    QByteArray buildFloatFrame(quint8 cmd1, quint8 cmd2, float value) const;
    QByteArray buildMotor2MitFrame() const;
    QByteArray buildFrame(quint8 cmd1, quint8 cmd2, const QByteArray &data) const;
    bool writeFrame(const QByteArray &frame);
    void writeNextQueuedFrame();
    void sendVerticalParam(quint8 cmd2, double value, const QString &name);
    void sendMotorRun(quint8 motorId, bool running);
    void updateRunButton(quint8 motorId, bool running);
    void setControlsEnabled(bool enabled);
    void appendLog(const QString &message);
    void setupRehabWorkflow();
    void showRehabPage(int page);
    void startRehabSession();
    void stopRehabSession(bool completed);
    void updateRehabSessionClock();
    QByteArray buildFlexRehabFrame(quint8 cmd2, const QByteArray &data) const;
    bool sendFlexRehabStart();
    void sendFlexRehabPause(bool paused);
    void sendFlexRehabStop();
    // 被动康复
    QByteArray buildPassiveFrame(quint8 cmd2, const QByteArray &data) const;
    void sendPassiveStart();
    void sendPassiveStop();
    bool sendPassiveParams();
    void sendManualEmergencyReset();

    Ui::MainWindow *ui;
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QVector<QBluetoothDeviceInfo> m_devices;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyCharacteristic m_rxCharacteristic;
    QLowEnergyCharacteristic m_txCharacteristic;
    QQueue<QByteArray> m_writeQueue;
    bool m_writeInProgress = false;
    bool m_verticalRunning = false;
    bool m_horizontalRunning = false;
    bool m_passiveRunning = false;
    bool m_rehabActiveMode = false;
    bool m_rehabRunning = false;
    bool m_rehabPaused = false;
    int m_rehabElapsedSeconds = 0;
    QStackedWidget *m_rehabPages = nullptr;
    QLabel *m_rehabBreadcrumb = nullptr;
    QLabel *m_rehabModeTitle = nullptr;
    QLabel *m_rehabSpeedLabel = nullptr;
    QLabel *m_rehabResistanceLabel = nullptr;
    QLabel *m_rehabMotorLabel = nullptr;
    QLabel *m_rehabLiveMode = nullptr;
    QLabel *m_rehabLiveParams = nullptr;
    QLabel *m_rehabLiveTime = nullptr;
    QLabel *m_rehabCycleProgress = nullptr;
    QLabel *m_rehabJointSpeed = nullptr;
    QLabel *m_rehabGearSpeed = nullptr;
    QLabel *m_rehabJointForce = nullptr;
    QLabel *m_rehabGearForce = nullptr;
    QComboBox *m_rehabSpeedCombo = nullptr;
    QComboBox *m_rehabResistanceCombo = nullptr;
    QComboBox *m_rehabMotorCombo = nullptr;
    QDoubleSpinBox *m_rehabRangeMinSpin = nullptr;
    QDoubleSpinBox *m_rehabRangeMaxSpin = nullptr;
    QSpinBox *m_rehabCyclesSpin = nullptr;
    QPushButton *m_rehabPauseButton = nullptr;
    QTimer *m_rehabTimer = nullptr;
};
#endif // MAINWINDOW_H
