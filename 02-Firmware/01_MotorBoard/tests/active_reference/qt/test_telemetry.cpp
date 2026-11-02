#include <QtWidgets>
#include <QtCharts>
#include <QSerialPort>
#include <cstdint>
#include <cstring>
#include <cstdio>
// Test-only access: compile original sources unmodified; no production test hooks.
#define private public
#include "mainwindow.h"
#include "protocolworker.h"
#undef private
#include "floatiddefs.h"
#include "../../../app/active_reference.h"

static int checks;
#define CHECK(x) do { ++checks; if(!(x)) qFatal("FAIL line %d: %s",__LINE__,#x); } while(0)
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    // Offscreen platform does not discover Windows fonts automatically.
    int fontId=QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    CHECK(fontId>=0);
    app.setFont(QFont(QFontDatabase::applicationFontFamilies(fontId).first(),9));
    MainWindow window;
    ProtocolWorker receiver; // no openPort, no actual serial I/O
    int sent=0, received=0;
    QObject::connect(&receiver,&ProtocolWorker::rawDataSent,[&](const QByteArray&){++sent;});
    QObject::connect(window.protoWorker,&ProtocolWorker::rawDataSent,[&](const QByteArray&){++sent;});
    QObject::connect(&receiver,&ProtocolWorker::telemetryReceived,&window,
        [&](const QMap<uint8_t,float>& values){++received; window.onTelemetryReceived(values);});
    window.m_recording=true;
    for(auto *box:window.rehabM1Ctx.checkboxes) box->setChecked(true);
    for(auto *box:window.rehabM2Ctx.checkboxes) box->setChecked(true);
    for(auto *tabs:window.rehabPage->findChildren<QTabWidget*>()) tabs->setCurrentIndex(1);
    actv_rehab_t m1={},m2={};
    actv_ref_reset(&m1,-30,2); actv_ref_reset(&m2,-15,0.4f);
    for(int tick=1;tick<=1700;++tick) {
        actv_ref_tick(&m1,-30,30,30,2);
        actv_ref_tick(&m2,-15,25,17,0.4f);
        CHECK(m1.status==ACTV_REF_OK && m2.status==ACTV_REF_OK);
        if(tick%5) continue;
        // Synthetic feedback and control inputs are deliberately different from reference.
        QMap<uint8_t,float> values={{0x01,3.0f},{0x02,-4.0f},{0x0B,m1.pos},
                                  {0x11,7.0f},{0x12,-8.0f},{0x1D,m2.pos}};
        QByteArray payload; payload.append(char(values.size()));
        for(auto it=values.cbegin();it!=values.cend();++it) {
            payload.append(char(it.key())); uint32_t bits; float v=it.value();
            std::memcpy(&bits,&v,4);
            for(unsigned b=0;b<4;++b) payload.append(char((bits>>(8*b))&255));
        }
        auto frame=receiver.buildFrame(CMD_TELEMETRY,payload);
        CHECK(receiver.parseFrame(reinterpret_cast<const uint8_t*>(frame.constData()),frame.size())==frame.size());
        CHECK(QMetaObject::invokeMethod(window.mChartTimer,"timeout",Qt::DirectConnection));
        CHECK(window.rehabM1Ctx.buffers[0x0B].last().y()==m1.pos);
        CHECK(window.rehabM2Ctx.buffers[0x1D].last().y()==m2.pos);
        CHECK(window.rehabM1Ctx.series[0x0B]->isVisible());
        CHECK(window.rehabM2Ctx.series[0x1D]->isVisible());
        CHECK(window.rehabM1Ctx.series[0x0B]->points().last().y()==m1.pos);
        CHECK(window.rehabM2Ctx.series[0x1D]->points().last().y()==m2.pos);
        CHECK(window.rehabM1Ctx.buffers[0x02].last().y()==-4.0);
        CHECK(window.rehabM2Ctx.buffers[0x12].last().y()==-8.0);
        CHECK(window.rehabM1Ctx.buffers[0x01].last().y()==3.0);
        CHECK(window.rehabM2Ctx.buffers[0x11].last().y()==7.0);
        CHECK(window.m_recordBuffer.last().values==values);
    }
    CHECK(received==340); CHECK(sent==0);
    CHECK(!receiver.isOpen() && !window.protoWorker->isOpen());
    CHECK(window.rehabM1Ctx.buffers[0x0B].size()==200);
    CHECK(window.rehabM2Ctx.buffers[0x1D].size()==200);
    window.m_recording=false;
    window.showRehabPage(); window.resize(1500,1000); window.show();
    QApplication::processEvents();
    CHECK(window.grab().save("qt-offline-preview.png"));
    std::printf("PASS %d checks; 340 generated telemetry frames; original parser/recording/charts; zero sends; no open ports\n",checks);
}
