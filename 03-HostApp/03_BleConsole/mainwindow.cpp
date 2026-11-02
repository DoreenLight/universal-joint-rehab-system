#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothPermission>
#include <QBluetoothUuid>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLowEnergyController>
#include <QLowEnergyDescriptor>
#include <QLowEnergyService>
#include <QMessageBox>
#include <QPermission>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace {
constexpr char kTargetDeviceName[] = "Kangfu-RS485";
const QBluetoothUuid kServiceUuid(QStringLiteral("5b12fdaf-479a-4722-a0f3-b993592377b2"));
const QBluetoothUuid kRxUuid(QStringLiteral("768a019c-a143-40c8-834d-d515c433e006"));
const QBluetoothUuid kTxUuid(QStringLiteral("768a019c-a143-40c8-834d-d515c433e007"));
const QBluetoothUuid kCccdUuid(QStringLiteral("00002902-0000-1000-8000-00805f9b34fb"));

constexpr int kFrameLength = 14;
constexpr quint8 kForceSensorCmdGroup = 0x21;
constexpr quint8 kForceSensorZeroCmd = 0x01;

quint16 mapToUInt(double value, double minValue, double maxValue, quint16 maxInt)
{
    const double clipped = std::clamp(value, minValue, maxValue);
    const double normalized = (clipped - minValue) / (maxValue - minValue);
    return static_cast<quint16>(normalized * static_cast<double>(maxInt));
}

QString frameToHex(const QByteArray &frame)
{
    return QString::fromLatin1(frame.toHex(' ').toUpper());
}

QString discoveryMethodsToString(QBluetoothDeviceDiscoveryAgent::DiscoveryMethods methods)
{
    QStringList names;
    if (methods & QBluetoothDeviceDiscoveryAgent::ClassicMethod) {
        names << QStringLiteral("Classic");
    }
    if (methods & QBluetoothDeviceDiscoveryAgent::LowEnergyMethod) {
        names << QStringLiteral("LowEnergy");
    }
    return names.isEmpty() ? QStringLiteral("None") : names.join(QStringLiteral(", "));
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUiDefaults();
    setupRehabWorkflow();
    setupConnections();
}

MainWindow::~MainWindow()
{
    resetBleConnection();
    delete ui;
}

void MainWindow::setupUiDefaults()
{
    auto configureSpin = [](QDoubleSpinBox *spin, double minValue, double maxValue, double step) {
        spin->setDecimals(3);
        spin->setRange(minValue, maxValue);
        spin->setSingleStep(step);
        spin->setValue(0.0);
    };

    configureSpin(ui->verticalAngleSpin, -10000.0, 10000.0, 0.1);
    configureSpin(ui->verticalVelocitySpin, -10000.0, 10000.0, 0.1);
    configureSpin(ui->verticalCurrentSpin, -10000.0, 10000.0, 0.1);
    configureSpin(ui->verticalKpSpin, -10000.0, 10000.0, 0.1);
    configureSpin(ui->verticalKiSpin, -10000.0, 10000.0, 0.1);

    configureSpin(ui->horizontalAngleSpin, -12.57, 12.57, 0.01);
    configureSpin(ui->horizontalVelocitySpin, -33.0, 33.0, 0.1);
    configureSpin(ui->horizontalTorqueSpin, -14.0, 14.0, 0.1);
    configureSpin(ui->horizontalKpSpin, 0.0, 500.0, 1.0);
    configureSpin(ui->horizontalKdSpin, 0.0, 5.0, 0.01);

    updateRunButton(1, false);
    updateRunButton(2, false);
    setControlsEnabled(false);
    ui->connectButton->setEnabled(false);
    ui->bleStatusLabel->setStyleSheet(QStringLiteral("color: #8A4B00; font-weight: 600;"));
    appendLog(QStringLiteral("Click Search. The app will auto-connect when %1 is found.").arg(QString::fromLatin1(kTargetDeviceName)));
}

void MainWindow::setupRehabWorkflow()
{
    auto *root = new QWidget(ui->tabWidget);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(24, 18, 24, 18);
    rootLayout->setSpacing(14);

    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("腕康复训练"));
    title->setObjectName(QStringLiteral("rehabTitle"));
    m_rehabBreadcrumb = new QLabel(QStringLiteral("开始训练"));
    m_rehabBreadcrumb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_rehabBreadcrumb);
    rootLayout->addLayout(header);

    m_rehabPages = new QStackedWidget(root);
    rootLayout->addWidget(m_rehabPages, 1);

    const QString cardStyle = QStringLiteral(
        "QPushButton { min-height: 76px; border: 1px solid #B9D4F4; border-radius: 12px;"
        "background: white; color: #173B66; font-size: 18px; font-weight: 600; padding: 12px; }"
        "QPushButton:hover { border: 2px solid #2F80ED; background: #EEF6FF; }"
        "QPushButton:pressed { background: #DCEEFF; }");
    const QString primaryStyle = QStringLiteral(
        "QPushButton { min-height: 44px; border: 0; border-radius: 10px; background: #2F80ED;"
        "color: white; font-size: 16px; font-weight: 700; padding: 8px 24px; }"
        "QPushButton:hover { background: #2568C7; }");
    const QString secondaryStyle = QStringLiteral(
        "QPushButton { min-height: 40px; border: 1px solid #7BADE8; border-radius: 9px;"
        "background: white; color: #245E9C; font-size: 15px; font-weight: 600; padding: 7px 18px; }"
        "QPushButton:hover { background: #EEF6FF; }");
    const QString dangerStyle = QStringLiteral(
        "QPushButton { min-height: 40px; border: 0; border-radius: 9px; background: #D64545;"
        "color: white; font-size: 15px; font-weight: 700; padding: 7px 18px; }"
        "QPushButton:hover { background: #B93535; }");

    auto makePage = [this](const QString &heading, const QString &hint) {
        auto *page = new QWidget(m_rehabPages);
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(34, 24, 34, 24);
        layout->setSpacing(16);
        auto *headingLabel = new QLabel(heading);
        headingLabel->setObjectName(QStringLiteral("pageHeading"));
        headingLabel->setAlignment(Qt::AlignCenter);
        auto *hintLabel = new QLabel(hint);
        hintLabel->setObjectName(QStringLiteral("pageHint"));
        hintLabel->setAlignment(Qt::AlignCenter);
        hintLabel->setWordWrap(true);
        layout->addStretch();
        layout->addWidget(headingLabel);
        layout->addWidget(hintLabel);
        return qMakePair(page, layout);
    };

    {
        const auto page = makePage(QStringLiteral("准备开始今天的腕关节康复"),
                                   QStringLiteral("连接设备后，按流程选择训练方向和训练方式。"));
        auto *start = new QPushButton(QStringLiteral("开始康复训练"));
        start->setStyleSheet(primaryStyle);
        page.second->addWidget(start, 0, Qt::AlignHCenter);
        page.second->addStretch();
        connect(start, &QPushButton::clicked, this, [this]() { showRehabPage(1); });
        m_rehabPages->addWidget(page.first);
    }

    {
        const auto page = makePage(QStringLiteral("请选择康复训练方向"),
                                   QStringLiteral("第一版先完成蓝色流程：掌屈、背伸。"));
        auto *grid = new QGridLayout;
        auto *flex = new QPushButton(QStringLiteral("掌屈、背伸\n进入训练设置"));
        flex->setStyleSheet(cardStyle);
        auto *ulnar = new QPushButton(QStringLiteral("尺偏、桡偏\n后续开放"));
        auto *rotation = new QPushButton(QStringLiteral("旋内、旋外\n后续开放"));
        auto *circle = new QPushButton(QStringLiteral("环转运动\n后续开放"));
        for (auto *button : {ulnar, rotation, circle}) {
            button->setStyleSheet(cardStyle);
            button->setEnabled(false);
        }
        grid->addWidget(flex, 0, 0);
        grid->addWidget(ulnar, 0, 1);
        grid->addWidget(rotation, 1, 0);
        grid->addWidget(circle, 1, 1);
        page.second->addLayout(grid);
        auto *back = new QPushButton(QStringLiteral("返回"));
        back->setStyleSheet(secondaryStyle);
        page.second->addWidget(back, 0, Qt::AlignLeft);
        page.second->addStretch();
        connect(flex, &QPushButton::clicked, this, [this]() { showRehabPage(2); });
        connect(back, &QPushButton::clicked, this, [this]() { showRehabPage(0); });
        m_rehabPages->addWidget(page.first);
    }

    {
        const auto page = makePage(QStringLiteral("掌屈、背伸康复训练"),
                                   QStringLiteral("被动训练由设备带动；主动训练由用户克服设定阻力完成运动。"));
        auto *passive = new QPushButton(QStringLiteral("被动康复训练\n设备按设定速度带动运动"));
        auto *active = new QPushButton(QStringLiteral("主动康复训练\n用户主动驱动电机并克服阻力"));
        passive->setStyleSheet(cardStyle);
        active->setStyleSheet(cardStyle);
        page.second->addWidget(passive);
        page.second->addWidget(active);
        auto *back = new QPushButton(QStringLiteral("返回"));
        back->setStyleSheet(secondaryStyle);
        page.second->addWidget(back, 0, Qt::AlignLeft);
        page.second->addStretch();
        connect(passive, &QPushButton::clicked, this, [this]() {
            m_rehabActiveMode = false;
            showRehabPage(3);
        });
        connect(active, &QPushButton::clicked, this, [this]() {
            m_rehabActiveMode = true;
            showRehabPage(3);
        });
        connect(back, &QPushButton::clicked, this, [this]() { showRehabPage(1); });
        m_rehabPages->addWidget(page.first);
    }

    {
        auto *page = new QWidget(m_rehabPages);
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(60, 26, 60, 26);
        layout->setSpacing(15);
        m_rehabModeTitle = new QLabel;
        m_rehabModeTitle->setObjectName(QStringLiteral("pageHeading"));
        m_rehabModeTitle->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_rehabModeTitle);
        auto *form = new QGridLayout;
        form->setHorizontalSpacing(20);
        form->setVerticalSpacing(14);
        m_rehabCyclesSpin = new QSpinBox;
        m_rehabCyclesSpin->setRange(1, 99);
        m_rehabCyclesSpin->setValue(6);
        m_rehabRangeMinSpin = new QDoubleSpinBox;
        m_rehabRangeMinSpin->setRange(-90.0, 90.0);
        m_rehabRangeMinSpin->setDecimals(1);
        m_rehabRangeMinSpin->setSuffix(QStringLiteral("°"));
        m_rehabRangeMinSpin->setValue(-30.0);
        m_rehabRangeMaxSpin = new QDoubleSpinBox;
        m_rehabRangeMaxSpin->setRange(-90.0, 90.0);
        m_rehabRangeMaxSpin->setDecimals(1);
        m_rehabRangeMaxSpin->setSuffix(QStringLiteral("°"));
        m_rehabRangeMaxSpin->setValue(30.0);
        m_rehabSpeedCombo = new QComboBox;
        m_rehabSpeedCombo->addItems({QStringLiteral("20（低速档）"),
                                     QStringLiteral("40（中速档）"),
                                     QStringLiteral("80（高速档）")});
        m_rehabSpeedCombo->setCurrentIndex(1);
        m_rehabResistanceCombo = new QComboBox;
        m_rehabResistanceCombo->addItems({QStringLiteral("低阻力"),
                                          QStringLiteral("中阻力"),
                                          QStringLiteral("高阻力")});
        m_rehabResistanceCombo->setCurrentIndex(1);
        m_rehabMotorCombo = new QComboBox;
        m_rehabMotorCombo->addItems({QStringLiteral("关节电机 + 齿轮电机"),
                                     QStringLiteral("仅关节电机"),
                                     QStringLiteral("仅齿轮电机")});
        m_rehabSpeedLabel = new QLabel(QStringLiteral("训练速度"));
        form->addWidget(new QLabel(QStringLiteral("训练组数")), 0, 0);
        form->addWidget(m_rehabCyclesSpin, 0, 1);
        form->addWidget(m_rehabSpeedLabel, 1, 0);
        form->addWidget(m_rehabSpeedCombo, 1, 1);
        form->addWidget(new QLabel(QStringLiteral("转动区间")), 2, 0);
        auto *rangeLayout = new QHBoxLayout;
        rangeLayout->addWidget(m_rehabRangeMinSpin);
        rangeLayout->addWidget(new QLabel(QStringLiteral("至")));
        rangeLayout->addWidget(m_rehabRangeMaxSpin);
        form->addLayout(rangeLayout, 2, 1);
        m_rehabResistanceLabel = new QLabel(QStringLiteral("辅助阻力"));
        form->addWidget(m_rehabResistanceLabel, 3, 0);
        form->addWidget(m_rehabResistanceCombo, 3, 1);
        m_rehabMotorLabel = new QLabel(QStringLiteral("训练电机"));
        form->addWidget(m_rehabMotorLabel, 4, 0);
        form->addWidget(m_rehabMotorCombo, 4, 1);
        layout->addLayout(form);
        auto *note = new QLabel(QStringLiteral(
            "主动模式的阻力用于提供可控阻尼，不会主动拖动患者。实际力矩值需在固件安全限幅完成后启用。"));
        note->setObjectName(QStringLiteral("safetyNote"));
        note->setWordWrap(true);
        layout->addWidget(note);
        auto *buttons = new QHBoxLayout;
        auto *back = new QPushButton(QStringLiteral("返回"));
        auto *start = new QPushButton(QStringLiteral("开始训练"));
        back->setStyleSheet(secondaryStyle);
        start->setStyleSheet(primaryStyle);
        buttons->addWidget(back);
        buttons->addStretch();
        buttons->addWidget(start);
        layout->addLayout(buttons);
        layout->addStretch();
        connect(back, &QPushButton::clicked, this, [this]() { showRehabPage(2); });
        connect(start, &QPushButton::clicked, this, &MainWindow::startRehabSession);
        m_rehabPages->addWidget(page);
    }

    {
        auto *page = new QWidget(m_rehabPages);
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(50, 28, 50, 28);
        layout->setSpacing(16);
        m_rehabLiveMode = new QLabel;
        m_rehabLiveMode->setObjectName(QStringLiteral("pageHeading"));
        m_rehabLiveMode->setAlignment(Qt::AlignCenter);
        m_rehabLiveParams = new QLabel;
        m_rehabLiveParams->setObjectName(QStringLiteral("livePanel"));
        m_rehabLiveParams->setAlignment(Qt::AlignCenter);
        m_rehabLiveParams->setWordWrap(true);
        auto *progressRow = new QHBoxLayout;
        auto *progressTitle = new QLabel(QStringLiteral("循环完成度"));
        progressTitle->setObjectName(QStringLiteral("metricTitle"));
        m_rehabCycleProgress = new QLabel(QStringLiteral("0 / 0"));
        m_rehabCycleProgress->setObjectName(QStringLiteral("cycleProgress"));
        m_rehabCycleProgress->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        progressRow->addWidget(progressTitle);
        progressRow->addStretch();
        progressRow->addWidget(m_rehabCycleProgress);

        auto makeMetricCard = [](const QString &title, QLabel **valueLabel) {
            auto *card = new QFrame;
            card->setObjectName(QStringLiteral("metricCard"));
            auto *cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(14, 12, 14, 12);
            auto *name = new QLabel(title);
            name->setObjectName(QStringLiteral("metricTitle"));
            auto *value = new QLabel(QStringLiteral("等待设备数据"));
            value->setObjectName(QStringLiteral("metricValue"));
            value->setAlignment(Qt::AlignCenter);
            value->setWordWrap(true);
            cardLayout->addWidget(name);
            cardLayout->addWidget(value, 1);
            *valueLabel = value;
            return card;
        };
        auto *metrics = new QGridLayout;
        metrics->setSpacing(12);
        metrics->addWidget(makeMetricCard(QStringLiteral("关节电机即时转速"),
                                          &m_rehabJointSpeed), 0, 0);
        metrics->addWidget(makeMetricCard(QStringLiteral("齿轮电机即时转速"),
                                          &m_rehabGearSpeed), 0, 1);
        metrics->addWidget(makeMetricCard(QStringLiteral("关节电机受力"),
                                          &m_rehabJointForce), 1, 0);
        metrics->addWidget(makeMetricCard(QStringLiteral("齿轮电机受力"),
                                          &m_rehabGearForce), 1, 1);
        m_rehabLiveTime = new QLabel(QStringLiteral("康复时长  00:00"));
        m_rehabLiveTime->setObjectName(QStringLiteral("liveTime"));
        m_rehabLiveTime->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_rehabLiveMode);
        layout->addWidget(m_rehabLiveParams);
        layout->addLayout(progressRow);
        layout->addLayout(metrics, 1);
        layout->addWidget(m_rehabLiveTime);
        auto *buttons = new QHBoxLayout;
        m_rehabPauseButton = new QPushButton(QStringLiteral("暂停"));
        auto *finish = new QPushButton(QStringLiteral("结束训练"));
        auto *reset = new QPushButton(QStringLiteral("紧急复位到 0°"));
        m_rehabPauseButton->setStyleSheet(secondaryStyle);
        finish->setStyleSheet(secondaryStyle);
        reset->setStyleSheet(dangerStyle);
        buttons->addWidget(m_rehabPauseButton);
        buttons->addWidget(finish);
        buttons->addStretch();
        buttons->addWidget(reset);
        layout->addLayout(buttons);
        layout->addStretch();
        connect(m_rehabPauseButton, &QPushButton::clicked, this, [this]() {
            m_rehabPaused = !m_rehabPaused;
            m_rehabPauseButton->setText(m_rehabPaused ? QStringLiteral("继续") : QStringLiteral("暂停"));
            if (!m_rehabActiveMode) {
                sendFlexRehabPause(m_rehabPaused);
            }
            appendLog(m_rehabPaused ? QStringLiteral("训练已暂停。") : QStringLiteral("训练已继续。"));
        });
        connect(finish, &QPushButton::clicked, this, [this]() { stopRehabSession(false); });
        connect(reset, &QPushButton::clicked, this, &MainWindow::sendManualEmergencyReset);
        m_rehabPages->addWidget(page);
    }

    m_rehabTimer = new QTimer(this);
    m_rehabTimer->setInterval(1000);
    connect(m_rehabTimer, &QTimer::timeout, this, &MainWindow::updateRehabSessionClock);

    root->setStyleSheet(QStringLiteral(
        "QWidget { font-family: 'Microsoft YaHei UI'; color: #183B5B; }"
        "QWidget#rehabWorkflow { background: #F4F8FD; }"
        "QLabel#rehabTitle { font-size: 24px; font-weight: 800; color: #164A7B; }"
        "QLabel#pageHeading { font-size: 22px; font-weight: 800; color: #164A7B; padding: 8px; }"
        "QLabel#pageHint { font-size: 14px; color: #66809A; padding-bottom: 10px; }"
        "QLabel#safetyNote { background: #FFF7E6; color: #8A5A00; border: 1px solid #F2D28D;"
        "border-radius: 8px; padding: 10px; }"
        "QLabel#livePanel { background: white; border: 1px solid #B9D4F4; border-radius: 12px;"
        "font-size: 17px; padding: 22px; }"
        "QLabel#liveTime { font-size: 20px; font-weight: 700; color: #2F80ED; padding: 12px; }"
        "QFrame#metricCard { background: white; border: 1px solid #B9D4F4; border-radius: 10px; }"
        "QLabel#metricTitle { color: #56728D; font-size: 14px; font-weight: 600; }"
        "QLabel#metricValue { color: #174B7A; font-size: 20px; font-weight: 800; padding: 8px; }"
        "QLabel#cycleProgress { color: #2F80ED; font-size: 24px; font-weight: 900; padding: 4px 12px; }"
        "QComboBox, QSpinBox { min-height: 36px; border: 1px solid #AFC9E6; border-radius: 7px;"
        "background: white; padding: 2px 10px; font-size: 15px; }"));
    root->setObjectName(QStringLiteral("rehabWorkflow"));
    ui->tabWidget->insertTab(0, root, QStringLiteral("康复训练"));
    ui->tabWidget->setCurrentWidget(root);
    showRehabPage(0);
}

void MainWindow::showRehabPage(int page)
{
    if (m_rehabPages == nullptr || page < 0 || page >= m_rehabPages->count()) {
        return;
    }
    static const QStringList crumbs = {
        QStringLiteral("开始训练"),
        QStringLiteral("选择方向"),
        QStringLiteral("选择方式"),
        QStringLiteral("训练参数"),
        QStringLiteral("实时训练")
    };
    m_rehabPages->setCurrentIndex(page);
    m_rehabBreadcrumb->setText(crumbs.value(page));
    if (page == 3) {
        m_rehabModeTitle->setText(m_rehabActiveMode
                                      ? QStringLiteral("掌屈、背伸主动康复训练")
                                      : QStringLiteral("掌屈、背伸被动康复训练"));
        m_rehabSpeedLabel->setText(m_rehabActiveMode
                                       ? QStringLiteral("辅助速度档")
                                       : QStringLiteral("电机速度档"));
        m_rehabSpeedLabel->setVisible(!m_rehabActiveMode);
        m_rehabSpeedCombo->setVisible(!m_rehabActiveMode);
        m_rehabResistanceCombo->setEnabled(m_rehabActiveMode);
        m_rehabMotorCombo->setEnabled(m_rehabActiveMode);
        m_rehabResistanceLabel->setVisible(m_rehabActiveMode);
        m_rehabResistanceCombo->setVisible(m_rehabActiveMode);
        m_rehabMotorLabel->setVisible(m_rehabActiveMode);
        m_rehabMotorCombo->setVisible(m_rehabActiveMode);
    }
}

void MainWindow::startRehabSession()
{
    if (m_rehabActiveMode) {
        QMessageBox::information(
            this,
            QStringLiteral("主动康复尚未启用"),
            QStringLiteral("主动阻力界面已完成，但双电机限力矩、暂停、结束和急停协议尚未完成安全验证。"
                           "当前不会向电机下发主动阻力命令。"));
        appendLog(QStringLiteral("主动康复启动被安全锁定：等待双电机阻力与急停协议。"));
        return;
    }

    const int speed = m_rehabSpeedCombo->currentIndex() == 0 ? 20
                      : (m_rehabSpeedCombo->currentIndex() == 1 ? 40 : 80);
    if (m_rehabRangeMinSpin->value() >= m_rehabRangeMaxSpin->value()) {
        QMessageBox::warning(this, QStringLiteral("参数错误"),
                             QStringLiteral("转动区间左端必须小于右端。"));
        return;
    }
    m_rehabElapsedSeconds = 0;
    m_rehabPaused = false;
    m_rehabPauseButton->setText(QStringLiteral("暂停"));
    m_rehabLiveMode->setText(m_rehabActiveMode
                                 ? QStringLiteral("掌屈、背伸主动康复训练")
                                 : QStringLiteral("掌屈、背伸被动康复训练"));
    m_rehabLiveParams->setText(
        m_rehabActiveMode
            ? QStringLiteral("训练组数：%1\n速度档：%2\n阻力：%3\n训练对象：%4")
                  .arg(m_rehabCyclesSpin->value()).arg(speed)
                  .arg(m_rehabResistanceCombo->currentText(), m_rehabMotorCombo->currentText())
            : QStringLiteral("训练组数：%1\n电机速度档：%2\n转动区间：%3° 至 %4°\n运动方向：掌屈 ↔ 背伸")
                  .arg(m_rehabCyclesSpin->value()).arg(speed)
                  .arg(m_rehabRangeMinSpin->value(), 0, 'f', 1)
                  .arg(m_rehabRangeMaxSpin->value(), 0, 'f', 1));
    m_rehabLiveTime->setText(QStringLiteral("康复时长  00:00"));
    m_rehabCycleProgress->setText(
        QStringLiteral("0 / %1").arg(m_rehabCyclesSpin->value()));
    m_rehabJointSpeed->setText(QStringLiteral("等待设备数据"));
    m_rehabGearSpeed->setText(QStringLiteral("等待设备数据"));
    m_rehabJointForce->setText(QStringLiteral("等待设备数据"));
    m_rehabGearForce->setText(QStringLiteral("等待设备数据"));

    if (!sendFlexRehabStart()) {
        return;
    }
    showRehabPage(4);
    m_rehabTimer->start();
}

void MainWindow::stopRehabSession(bool completed)
{
    m_rehabTimer->stop();
    if (!m_rehabActiveMode) {
        sendFlexRehabStop();
    }
    appendLog(completed ? QStringLiteral("本次康复训练已完成。")
                        : QStringLiteral("本次康复训练已结束。"));
    showRehabPage(0);
}

void MainWindow::updateRehabSessionClock()
{
    if (m_rehabPaused) {
        return;
    }
    ++m_rehabElapsedSeconds;
    const int minutes = m_rehabElapsedSeconds / 60;
    const int seconds = m_rehabElapsedSeconds % 60;
    m_rehabLiveTime->setText(
        QStringLiteral("康复时长  %1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0')));
}

void MainWindow::setupConnections()
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::startDeviceSearch);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectToSelectedDevice);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &MainWindow::addDiscoveredDevice);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, [this]() {
        ui->searchButton->setEnabled(true);
        appendLog(QStringLiteral("Device scan finished."));
    });
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, [this]() {
        ui->searchButton->setEnabled(true);
        appendLog(QStringLiteral("Device scan stopped."));
    });
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            [this](QBluetoothDeviceDiscoveryAgent::Error) {
                ui->searchButton->setEnabled(true);
                appendLog(QStringLiteral("Scan failed: %1").arg(m_discoveryAgent->errorString()));
            });

    connect(ui->sendVerticalAngleButton, &QPushButton::clicked, this, [this]() {
        sendVerticalParam(0x01, ui->verticalAngleSpin->value(), QStringLiteral("Vertical Angle"));
    });
    connect(ui->sendVerticalVelocityButton, &QPushButton::clicked, this, [this]() {
        sendVerticalParam(0x02, ui->verticalVelocitySpin->value(), QStringLiteral("Vertical Velocity"));
    });
    connect(ui->sendVerticalCurrentButton, &QPushButton::clicked, this, [this]() {
        sendVerticalParam(0x03, ui->verticalCurrentSpin->value(), QStringLiteral("Vertical Current"));
    });
    connect(ui->sendVerticalKpButton, &QPushButton::clicked, this, [this]() {
        sendVerticalParam(0x04, ui->verticalKpSpin->value(), QStringLiteral("Vertical KP"));
    });
    connect(ui->sendVerticalKiButton, &QPushButton::clicked, this, [this]() {
        sendVerticalParam(0x05, ui->verticalKiSpin->value(), QStringLiteral("Vertical KI"));
    });

    connect(ui->verticalStartButton, &QPushButton::clicked, this, [this]() {
        sendMotorRun(1, !m_verticalRunning);
    });
    connect(ui->sendHorizontalParamsButton, &QPushButton::clicked, this, [this]() {
        if (writeFrame(buildMotor2MitFrame())) {
            appendLog(QStringLiteral("Horizontal MIT parameters sent."));
        }
    });
    connect(ui->horizontalZeroButton, &QPushButton::clicked, this, [this]() {
        if (writeFrame(buildIntFrame(0x11, 0x03, 1))) {
            appendLog(QStringLiteral("Horizontal zero reset sent."));
        }
    });
    connect(ui->horizontalStartButton, &QPushButton::clicked, this, [this]() {
        sendMotorRun(2, !m_horizontalRunning);
    });
    connect(ui->forceZeroButton, &QPushButton::clicked, this, [this]() {
        if (writeFrame(buildIntFrame(kForceSensorCmdGroup, kForceSensorZeroCmd, 1))) {
            appendLog(QStringLiteral("Force sensor zero calibration sent."));
        }
    });
    // 被动康复切换按钮
    connect(ui->passiveToggleButton, &QPushButton::clicked, this, [this]() {
        if (m_passiveRunning) {
            sendPassiveStop();
        } else {
            if (sendPassiveParams()) {
                sendPassiveStart();
            }
        }
    });
    connect(ui->passiveEmergencyResetButton, &QPushButton::clicked,
            this, &MainWindow::sendManualEmergencyReset);
}

void MainWindow::startDeviceSearch()
{
#if QT_CONFIG(permissions)
    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        appendLog(QStringLiteral("Requesting Bluetooth permission..."));
        qApp->requestPermission(permission, this, [this](const QPermission &result) {
            if (result.status() == Qt::PermissionStatus::Granted) {
                startDeviceSearch();
            } else {
                appendLog(QStringLiteral("Bluetooth permission was not granted."));
                ui->bleStatusLabel->setText(QStringLiteral("No permission"));
                ui->searchButton->setEnabled(true);
            }
        });
        return;
    case Qt::PermissionStatus::Denied:
        appendLog(QStringLiteral("Bluetooth permission is denied by the system."));
        ui->bleStatusLabel->setText(QStringLiteral("No permission"));
        ui->searchButton->setEnabled(true);
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif

    if (m_discoveryAgent->isActive()) {
        m_discoveryAgent->stop();
    }

    const auto supportedMethods = QBluetoothDeviceDiscoveryAgent::supportedDiscoveryMethods();
    appendLog(QStringLiteral("Supported discovery methods: %1").arg(discoveryMethodsToString(supportedMethods)));
    if (!(supportedMethods & QBluetoothDeviceDiscoveryAgent::LowEnergyMethod)) {
        appendLog(QStringLiteral("LowEnergy BLE discovery is not supported by this Qt/Windows environment."));
        appendLog(QStringLiteral("Check Windows Bluetooth switch, BLE-capable adapter/driver, and Windows app Bluetooth permission."));
        ui->bleStatusLabel->setText(QStringLiteral("BLE unsupported"));
        ui->searchButton->setEnabled(true);
        return;
    }

    resetBleConnection();
    m_devices.clear();
    ui->deviceComboBox->clear();
    ui->connectButton->setEnabled(false);
    ui->searchButton->setEnabled(false);
    ui->bleStatusLabel->setText(QStringLiteral("Scanning"));
    appendLog(QStringLiteral("Scanning BLE devices..."));

    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void MainWindow::addDiscoveredDevice(const QBluetoothDeviceInfo &deviceInfo)
{
    if (!(deviceInfo.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) {
        return;
    }

    const QString name = deviceInfo.name().isEmpty() ? QStringLiteral("(unnamed)") : deviceInfo.name();
    const QString address = deviceInfo.address().toString();

    for (const QBluetoothDeviceInfo &knownDevice : std::as_const(m_devices)) {
        if (!address.isEmpty() && knownDevice.address().toString() == address) {
            return;
        }
        if (address.isEmpty() && knownDevice.name() == deviceInfo.name()) {
            return;
        }
    }

    const int index = m_devices.size();
    m_devices.push_back(deviceInfo);
    ui->deviceComboBox->addItem(QStringLiteral("%1  %2").arg(name, address), index);
    ui->connectButton->setEnabled(true);
    appendLog(QStringLiteral("Found device: %1 %2").arg(name, address));

    if (name == QString::fromLatin1(kTargetDeviceName)) {
        appendLog(QStringLiteral("Target device found. Stopping scan and connecting."));
        m_discoveryAgent->stop();
        connectToDevice(deviceInfo);
    }
}

void MainWindow::connectToSelectedDevice()
{
    const int comboIndex = ui->deviceComboBox->currentIndex();
    if (comboIndex < 0) {
        appendLog(QStringLiteral("Search and select a BLE device first."));
        return;
    }

    const int deviceIndex = ui->deviceComboBox->itemData(comboIndex).toInt();
    if (deviceIndex < 0 || deviceIndex >= m_devices.size()) {
        appendLog(QStringLiteral("Invalid device index. Please scan again."));
        return;
    }

    connectToDevice(m_devices.at(deviceIndex));
}

void MainWindow::connectToDevice(const QBluetoothDeviceInfo &deviceInfo)
{
    resetBleConnection();

    ui->bleStatusLabel->setText(QStringLiteral("Connecting"));
    appendLog(QStringLiteral("Connecting to %1").arg(deviceInfo.name()));

    m_controller = QLowEnergyController::createCentral(deviceInfo, this);
    QLowEnergyController *controller = m_controller;

    connect(controller, &QLowEnergyController::connected, this, [this, controller]() {
        if (controller != m_controller) {
            return;
        }
        ui->bleStatusLabel->setText(QStringLiteral("Discovering"));
        appendLog(QStringLiteral("BLE connected. Discovering services."));
        controller->discoverServices();
    });
    connect(controller, &QLowEnergyController::disconnected, this, [this, controller]() {
        if (controller != m_controller) {
            return;
        }
        ui->bleStatusLabel->setText(QStringLiteral("Disconnected"));
        ui->bleStatusLabel->setStyleSheet(QStringLiteral("color: #B00020; font-weight: 600;"));
        setControlsEnabled(false);
        appendLog(QStringLiteral("BLE disconnected."));
    });
    connect(controller, &QLowEnergyController::serviceDiscovered,
            this, &MainWindow::onServiceDiscovered);
    connect(controller, &QLowEnergyController::discoveryFinished,
            this, &MainWindow::onServiceScanDone);
    connect(controller, &QLowEnergyController::errorOccurred, this,
            [this, controller](QLowEnergyController::Error) {
                if (controller != m_controller) {
                    return;
                }
                ui->bleStatusLabel->setText(QStringLiteral("Error"));
                ui->bleStatusLabel->setStyleSheet(QStringLiteral("color: #B00020; font-weight: 600;"));
                appendLog(QStringLiteral("BLE controller error: %1").arg(controller->errorString()));
            });

    controller->connectToDevice();
}

void MainWindow::resetBleConnection()
{
    setControlsEnabled(false);
    m_rxCharacteristic = QLowEnergyCharacteristic();
    m_txCharacteristic = QLowEnergyCharacteristic();

    if (m_service != nullptr) {
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_controller != nullptr) {
        m_controller->disconnectFromDevice();
        m_controller->deleteLater();
        m_controller = nullptr;
    }
}

void MainWindow::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    if (serviceUuid == kServiceUuid) {
        appendLog(QStringLiteral("Control service found."));
    }
}

void MainWindow::onServiceScanDone()
{
    if (m_controller == nullptr) {
        return;
    }

    m_service = m_controller->createServiceObject(kServiceUuid, this);
    if (m_service == nullptr) {
        ui->bleStatusLabel->setText(QStringLiteral("No service"));
        ui->bleStatusLabel->setStyleSheet(QStringLiteral("color: #B00020; font-weight: 600;"));
        appendLog(QStringLiteral("ESP32 BLE service UUID was not found."));
        return;
    }

    connect(m_service, &QLowEnergyService::stateChanged,
            this, &MainWindow::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &MainWindow::onBleNotification);
    connect(m_service, &QLowEnergyService::characteristicWritten, this,
            [this](const QLowEnergyCharacteristic &, const QByteArray &value) {
                appendLog(QStringLiteral("BLE write done: %1").arg(frameToHex(value)));
                m_writeInProgress = false;
                writeNextQueuedFrame();
            });
    connect(m_service, &QLowEnergyService::errorOccurred, this,
            [this](QLowEnergyService::ServiceError error) {
                appendLog(QStringLiteral("BLE service error: %1").arg(static_cast<int>(error)));
            });

    ui->bleStatusLabel->setText(QStringLiteral("Characteristics"));
    appendLog(QStringLiteral("Discovering BLE characteristics."));
    m_service->discoverDetails();
}

void MainWindow::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    if (state != QLowEnergyService::RemoteServiceDiscovered || m_service == nullptr) {
        return;
    }

    m_rxCharacteristic = m_service->characteristic(kRxUuid);
    m_txCharacteristic = m_service->characteristic(kTxUuid);

    if (!m_rxCharacteristic.isValid()) {
        ui->bleStatusLabel->setText(QStringLiteral("No RX"));
        ui->bleStatusLabel->setStyleSheet(QStringLiteral("color: #B00020; font-weight: 600;"));
        appendLog(QStringLiteral("RX write characteristic was not found."));
        return;
    }

    if (m_txCharacteristic.isValid()) {
        const QLowEnergyDescriptor cccd = m_txCharacteristic.descriptor(kCccdUuid);
        if (cccd.isValid()) {
            m_service->writeDescriptor(cccd, QByteArray::fromHex("0100"));
            appendLog(QStringLiteral("ESP32 status notifications enabled."));
        }
    }

    ui->bleStatusLabel->setText(QStringLiteral("Connected"));
    ui->bleStatusLabel->setStyleSheet(QStringLiteral("color: #006B3C; font-weight: 700;"));
    setControlsEnabled(true);
    appendLog(QStringLiteral("BLE is ready. RS485 protocol frames can be sent."));
}

void MainWindow::onBleNotification(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() != kTxUuid || value.isEmpty()) {
        return;
    }

    const auto byteAt = [&value](int index) -> quint8 {
        return static_cast<quint8>(value.at(index));
    };

    if (value.size() >= 4 && byteAt(0) == 0xA5) {
        const quint8 status = byteAt(1);
        const quint8 detail = byteAt(2);
        if (status == 0x00) {
            appendLog(QStringLiteral("ESP32 accepted and forwarded %1 frame(s).").arg(detail));
        } else if (status == 0x01) {
            appendLog(QStringLiteral("ESP32 rejected the packet: length %1 is not a 14-byte multiple.").arg(detail));
        } else if (status == 0x02) {
            appendLog(QStringLiteral("ESP32 rejected frame %1: checksum/header failed.").arg(detail));
        } else {
            appendLog(QStringLiteral("ESP32 status: %1 %2").arg(status).arg(detail));
        }
        return;
    }

    if (value.size() >= 4 && byteAt(0) == 0xA6) {
        updateRunButton(byteAt(1), byteAt(2) != 0);
        return;
    }

    // Compact rehabilitation telemetry, little-endian signed int16:
    // A8, completed, total, flags, joint_speed*100, gear_speed*10,
    // joint_torque*100, gear_torque*100, XOR(bytes 0..11).
    if (value.size() >= 13 && byteAt(0) == 0xA8) {
        quint8 checksum = 0;
        for (int i = 0; i < 12; ++i) {
            checksum ^= byteAt(i);
        }
        if (checksum != byteAt(12)) {
            appendLog(QStringLiteral("康复遥测校验失败，数据未显示。"));
            return;
        }
        const auto int16At = [&value](int offset) {
            const quint16 bits = qFromLittleEndian<quint16>(
                reinterpret_cast<const uchar *>(value.constData() + offset));
            return static_cast<qint16>(bits);
        };
        const quint8 completed = byteAt(1);
        const quint8 total = byteAt(2);
        const float jointSpeed = int16At(4) / 100.0f;
        const float gearSpeed = int16At(6) / 10.0f;
        const float jointForce = int16At(8) / 100.0f;
        const float gearForce = int16At(10) / 100.0f;
        m_rehabCycleProgress->setText(
            QStringLiteral("%1 / %2").arg(completed).arg(total));
        m_rehabJointSpeed->setText(QStringLiteral("%1 rad/s").arg(jointSpeed, 0, 'f', 2));
        m_rehabGearSpeed->setText(QStringLiteral("%1 °/s").arg(gearSpeed, 0, 'f', 2));
        m_rehabJointForce->setText(QStringLiteral("%1 N·m").arg(jointForce, 0, 'f', 2));
        m_rehabGearForce->setText(QStringLiteral("%1 N·m").arg(gearForce, 0, 'f', 2));
        return;
    }

    if (value.size() >= 4 && byteAt(0) == 0xA7) {
        const quint8 event = byteAt(1);
        const quint8 result = byteAt(2);
        if (event == 0x30 && result == 0x00) {
            m_passiveRunning = false;
            ui->passiveToggleButton->setText(QString::fromUtf8("▶ 开始被动康复"));
            ui->passiveToggleButton->setStyleSheet(
                "QPushButton { background-color: #2F80ED; color: white; "
                "font-weight: bold; font-size: 14px; border-radius: 8px; padding: 8px; } "
                "QPushButton:hover { background-color: #2563EB; }");
            appendLog(QStringLiteral("被动康复已按设定循环自动完成。"));
        } else if (event == 0x30 && result == 0x01) {
            m_passiveRunning = false;
            ui->passiveToggleButton->setText(QString::fromUtf8("▶ 开始被动康复"));
            appendLog(QStringLiteral("被动康复循环已完成，但自动复位失败：IMU 未就绪。"));
        } else if (event == 0x30 && result == 0x02) {
            m_passiveRunning = false;
            ui->passiveToggleButton->setText(QString::fromUtf8("▶ 开始被动康复"));
            appendLog(QStringLiteral("被动康复循环已完成，但自动复位超时，电机已停止。"));
        } else if (event == 0x31 && result == 0x00) {
            appendLog(QStringLiteral("人工紧急复位完成：IMU roll 已回到 0°。"));
        } else if (event == 0x31 && result == 0x01) {
            appendLog(QStringLiteral("人工紧急复位被拒绝：IMU 未就绪，电机保持停止。"));
        } else if (event == 0x31 && result == 0x02) {
            appendLog(QStringLiteral("人工紧急复位超时：未到达 0°，电机已停止。"));
        } else if (event == 0x32 && result == 0x00) {
            m_rehabCycleProgress->setText(
                QStringLiteral("%1 / %1").arg(m_rehabCyclesSpin->value()));
            m_rehabRunning = false;
            stopRehabSession(true);
        } else if (event == 0x32 && result == 0x02) {
            m_rehabRunning = false;
            m_rehabTimer->stop();
            appendLog(QStringLiteral("训练完成，但自动复位超过 15 秒，电机已安全停止。"));
        } else {
            appendLog(QStringLiteral("STM32 status: event=%1 result=%2")
                          .arg(event, 2, 16, QLatin1Char('0'))
                          .arg(result, 2, 16, QLatin1Char('0')));
        }
        return;
    }

    appendLog(QStringLiteral("ESP32 notify: %1").arg(frameToHex(value)));
}

QByteArray MainWindow::buildIntFrame(quint8 cmd1, quint8 cmd2, qint32 value) const
{
    QByteArray data(8, '\0');
    const quint32 littleEndian = qToLittleEndian(static_cast<quint32>(value));
    std::memcpy(data.data(), &littleEndian, sizeof(littleEndian));
    return buildFrame(cmd1, cmd2, data);
}

QByteArray MainWindow::buildFloatFrame(quint8 cmd1, quint8 cmd2, float value) const
{
    QByteArray data(8, '\0');
    quint32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = qToLittleEndian(bits);
    std::memcpy(data.data(), &bits, sizeof(bits));
    return buildFrame(cmd1, cmd2, data);
}

QByteArray MainWindow::buildMotor2MitFrame() const
{
    QByteArray data(8, '\0');

    const quint16 angleInt = mapToUInt(ui->horizontalAngleSpin->value(), -12.57, 12.57, 65535);
    const quint16 velocityInt = mapToUInt(ui->horizontalVelocitySpin->value(), -33.0, 33.0, 4095);
    const quint16 kpInt = static_cast<quint16>((std::clamp(ui->horizontalKpSpin->value(), 0.0, 500.0) / 500.0) * 4095.0);
    const quint16 kdInt = static_cast<quint16>((std::clamp(ui->horizontalKdSpin->value(), 0.0, 5.0) / 5.0) * 4095.0);
    const quint16 torqueInt = mapToUInt(ui->horizontalTorqueSpin->value(), -14.0, 14.0, 4095);

    data[0] = static_cast<char>((angleInt >> 8) & 0xFF);
    data[1] = static_cast<char>(angleInt & 0xFF);
    data[2] = static_cast<char>((velocityInt >> 4) & 0xFF);
    data[3] = static_cast<char>(((velocityInt & 0x0F) << 4) | ((kpInt >> 8) & 0x0F));
    data[4] = static_cast<char>(kpInt & 0xFF);
    data[5] = static_cast<char>((kdInt >> 4) & 0xFF);
    data[6] = static_cast<char>(((kdInt & 0x0F) << 4) | ((torqueInt >> 8) & 0x0F));
    data[7] = static_cast<char>(torqueInt & 0xFF);

    return buildFrame(0x16, 0x01, data);
}

QByteArray MainWindow::buildFrame(quint8 cmd1, quint8 cmd2, const QByteArray &data) const
{
    QByteArray frame(kFrameLength, '\0');
    frame[0] = static_cast<char>(0x6D);
    frame[1] = static_cast<char>(0x6A);
    frame[2] = static_cast<char>(0x01);
    frame[3] = static_cast<char>(cmd1);
    frame[4] = static_cast<char>(cmd2);

    for (int i = 0; i < 8 && i < data.size(); ++i) {
        frame[5 + i] = data.at(i);
    }

    quint8 checksum = 0;
    for (int i = 2; i < 13; ++i) {
        checksum = static_cast<quint8>(checksum + static_cast<quint8>(frame.at(i)));
    }
    frame[13] = static_cast<char>(checksum);
    return frame;
}

bool MainWindow::writeFrame(const QByteArray &frame)
{
    if (m_service == nullptr || !m_rxCharacteristic.isValid()) {
        appendLog(QStringLiteral("ESP32 BLE is not connected."));
        return false;
    }

    if (frame.size() != kFrameLength) {
        appendLog(QStringLiteral("Internal error: frame length is not 14 bytes."));
        return false;
    }

    m_writeQueue.enqueue(frame);
    appendLog(QStringLiteral("TX queued: %1").arg(frameToHex(frame)));
    writeNextQueuedFrame();
    return true;
}

void MainWindow::writeNextQueuedFrame()
{
    if (m_writeInProgress || m_writeQueue.isEmpty() ||
        m_service == nullptr || !m_rxCharacteristic.isValid()) {
        return;
    }

    const QByteArray frame = m_writeQueue.dequeue();
    m_writeInProgress = true;
    m_service->writeCharacteristic(m_rxCharacteristic, frame,
                                   QLowEnergyService::WriteWithResponse);
    appendLog(QStringLiteral("TX: %1").arg(frameToHex(frame)));
}

void MainWindow::sendVerticalParam(quint8 cmd2, double value, const QString &name)
{
    if (writeFrame(buildFloatFrame(0x06, cmd2, static_cast<float>(value)))) {
        appendLog(QStringLiteral("%1 sent: %2").arg(name).arg(value));
    }
}

void MainWindow::sendMotorRun(quint8 motorId, bool running)
{
    const quint8 cmd1 = (motorId == 1) ? 0x01 : 0x11;
    if (writeFrame(buildIntFrame(cmd1, 0x01, running ? 1 : 0))) {
        updateRunButton(motorId, running);
        appendLog(QStringLiteral("%1 motor %2 command sent.")
                      .arg(motorId == 1 ? QStringLiteral("Vertical") : QStringLiteral("Horizontal"))
                      .arg(running ? QStringLiteral("start") : QStringLiteral("stop")));
    }
}

void MainWindow::updateRunButton(quint8 motorId, bool running)
{
    QPushButton *button = nullptr;
    if (motorId == 1) {
        m_verticalRunning = running;
        button = ui->verticalStartButton;
    } else if (motorId == 2) {
        m_horizontalRunning = running;
        button = ui->horizontalStartButton;
    } else {
        return;
    }

    button->setText(running ? QStringLiteral("Stop") : QStringLiteral("Start"));
    button->setStyleSheet(running
                              ? QStringLiteral("background-color: #C43E3E; color: white; font-weight: 700;")
                              : QStringLiteral("background-color: #2F80ED; color: white; font-weight: 700;"));
}

void MainWindow::setControlsEnabled(bool enabled)
{
    ui->sendVerticalAngleButton->setEnabled(enabled);
    ui->sendVerticalVelocityButton->setEnabled(enabled);
    ui->sendVerticalCurrentButton->setEnabled(enabled);
    ui->sendVerticalKpButton->setEnabled(enabled);
    ui->sendVerticalKiButton->setEnabled(enabled);
    ui->verticalStartButton->setEnabled(enabled);
    ui->sendHorizontalParamsButton->setEnabled(enabled);
    ui->horizontalZeroButton->setEnabled(enabled);
    ui->horizontalStartButton->setEnabled(enabled);
    ui->forceZeroButton->setEnabled(enabled);
}

void MainWindow::appendLog(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    ui->logTextBrowser->append(QStringLiteral("[%1] %2").arg(time, message));
}

// ====================================================================
// 被动康复功能
// ====================================================================

QByteArray MainWindow::buildFlexRehabFrame(quint8 cmd2, const QByteArray &data) const
{
    return buildFrame(0x32, cmd2, data);
}

bool MainWindow::sendFlexRehabStart()
{
    const float minDeg = static_cast<float>(m_rehabRangeMinSpin->value());
    const float maxDeg = static_cast<float>(m_rehabRangeMaxSpin->value());
    const float speedDegS = static_cast<float>(
        m_rehabSpeedCombo->currentIndex() == 0 ? 20 :
        (m_rehabSpeedCombo->currentIndex() == 1 ? 40 : 80));
    const quint32 cycles = static_cast<quint32>(m_rehabCyclesSpin->value());

    QByteArray rangePayload(8, '\0');
    std::memcpy(rangePayload.data(), &minDeg, sizeof(minDeg));
    std::memcpy(rangePayload.data() + 4, &maxDeg, sizeof(maxDeg));
    if (!writeFrame(buildFlexRehabFrame(0x03, rangePayload))) return false;

    const float resistanceNm = static_cast<float>(
        std::fabs(ui->horizontalTorqueSpin->value()));
    QByteArray resistancePayload(8, '\0');
    std::memcpy(resistancePayload.data(), &resistanceNm, sizeof(resistanceNm));
    if (!writeFrame(buildFlexRehabFrame(0x05, resistancePayload))) return false;

    QByteArray startPayload(8, '\0');
    std::memcpy(startPayload.data(), &speedDegS, sizeof(speedDegS));
    std::memcpy(startPayload.data() + 4, &cycles, sizeof(cycles));
    if (!writeFrame(buildFlexRehabFrame(0x01, startPayload))) return false;

    m_rehabRunning = true;
    appendLog(QStringLiteral("掌屈/背伸训练启动：关节电机，区间=%1° 至 %2°，速度=%3，循环=%4，阻力=%5 N·m")
                  .arg(minDeg, 0, 'f', 1).arg(maxDeg, 0, 'f', 1)
                  .arg(speedDegS, 0, 'f', 0).arg(cycles)
                  .arg(resistanceNm, 0, 'f', 2));
    return true;
}

void MainWindow::sendFlexRehabPause(bool paused)
{
    QByteArray payload(8, '\0');
    payload[0] = paused ? 1 : 0;
    writeFrame(buildFlexRehabFrame(0x04, payload));
}

void MainWindow::sendFlexRehabStop()
{
    if (!m_rehabRunning) return;
    QByteArray payload(8, '\0');
    if (writeFrame(buildFlexRehabFrame(0x02, payload))) {
        m_rehabRunning = false;
    }
}

QByteArray MainWindow::buildPassiveFrame(quint8 cmd2, const QByteArray &data) const
{
    return buildFrame(0x30, cmd2, data);
}

void MainWindow::sendPassiveStart()
{
    float speed = static_cast<float>(ui->passiveSpeedSpin->value());
    quint32 cycles = static_cast<quint32>(ui->passiveCyclesSpin->value());

    QByteArray payload(8, '0');
    memcpy(payload.data(), &speed, 4);
    memcpy(payload.data() + 4, &cycles, 4);

    if (writeFrame(buildPassiveFrame(0x01, payload))) {
        QByteArray dirPayload(8, '\0');
        const quint8 dir = static_cast<quint8>(ui->passiveDirCombo->currentIndex() + 1);
        dirPayload[0] = static_cast<char>(dir);
        writeFrame(buildPassiveFrame(0x05, dirPayload));
        m_passiveRunning = true;
        ui->passiveToggleButton->setText(QString::fromUtf8("■ 停止被动康复"));
        ui->passiveToggleButton->setStyleSheet("QPushButton { background-color: #C43E3E; color: white; font-weight: bold; font-size: 14px; border-radius: 8px; padding: 8px; } QPushButton:hover { background-color: #A83232; }");
        appendLog(QString("被动康复启动: 速度=%1, 循环=%2").arg(speed).arg(cycles));
    }
}

void MainWindow::sendPassiveStop()
{
    QByteArray payload(8, '0');
    if (writeFrame(buildPassiveFrame(0x02, payload))) {
        m_passiveRunning = false;
        ui->passiveToggleButton->setText(QString::fromUtf8("▶ 开始被动康复"));
        ui->passiveToggleButton->setStyleSheet("QPushButton { background-color: #2F80ED; color: white; font-weight: bold; font-size: 14px; border-radius: 8px; padding: 8px; } QPushButton:hover { background-color: #2563EB; }");
        appendLog("被动康复停止");
    }
}

bool MainWindow::sendPassiveParams()
{
    // 设置运动范围
    float minDeg = static_cast<float>(ui->passiveRangeMinSpin->value());
    float maxDeg = static_cast<float>(ui->passiveRangeMaxSpin->value());
    if (minDeg >= maxDeg) {
        appendLog(QStringLiteral("参数无效：角度范围左端必须小于右端。"));
        return false;
    }
    QByteArray rangePayload(8, '\0');
    memcpy(rangePayload.data(), &minDeg, 4);
    memcpy(rangePayload.data() + 4, &maxDeg, 4);
    writeFrame(buildPassiveFrame(0x03, rangePayload));

    // 设置暂停时间
    quint32 pauseMs = static_cast<quint32>(ui->passivePauseSpin->value());
    QByteArray pausePayload(8, '\0');
    memcpy(pausePayload.data(), &pauseMs, 4);
    writeFrame(buildPassiveFrame(0x04, pausePayload));

    // 设置方向
    quint8 dir = static_cast<quint8>(ui->passiveDirCombo->currentIndex() + 1);
    QByteArray dirPayload(8, '\0');
    dirPayload[0] = static_cast<char>(dir);
    writeFrame(buildPassiveFrame(0x05, dirPayload));
    return true;
}

void MainWindow::sendManualEmergencyReset()
{
    QByteArray payload(8, '\0');
    m_writeQueue.clear();
    if (writeFrame(buildPassiveFrame(0x06, payload))) {
        m_passiveRunning = false;
        ui->passiveToggleButton->setText(QString::fromUtf8("▶ 开始被动康复"));
        appendLog(QStringLiteral("人工紧急复位：已请求立即停止 Passive 并按 IMU 返回 0°"));
    }
}
