#include <QtWidgets>
#include <QtCharts>
#include <QSerialPort>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
// Access only in this test translation unit; production has no test hooks.
#define private public
#include "mainwindow.h"
#include "protocolworker.h"
#undef private
#include "ui_mainwindow.h"
#include "mainwindow.cpp"
#include "build/legacy-reference.inc"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) qFatal("FAIL line %d: %s", __LINE__, #x); } while (0)
static bool near(qreal a, qreal b, qreal eps=1e-9) { return std::abs(a-b)<=eps; }
static void waitEvents(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}
static void referenceMath() {
    const float nan=std::numeric_limits<float>::quiet_NaN();
    struct Case { float a,b; qreal seconds; float pause; int n; };
    const Case cases[]={{-30,30,6,2,5},{-12,23,9,.4f,3},{25,-15,12,0,2},
        {5,5,6,2,5},{5,5,6,0,1},{0,.005f,1,.015f,20},
        {-90,80,120,3,20},{0,45,2,0,1},{-30,30,0,2,0},
        {nan,30,nan,-1,21},{-30,nan,6,nan,5}};
    for (const auto &c:cases) {
        const qreal a=qIsFinite(c.a)?c.a:-30, b=qIsFinite(c.b)?c.b:30;
        const qreal duration=qIsFinite(c.seconds)&&c.seconds>0?c.seconds:6;
        const qreal pause=qIsFinite(c.pause)&&c.pause>=0?c.pause:0;
        const int cycles=qBound(1,c.n,20);
        qreal expectedT=0,newT=0;
        QList<QPointF> boundaries={QPointF(0,a)};
        for(int cycle=0;cycle<cycles;++cycle) {
            expectedT+=pause; boundaries.append(QPointF(expectedT,a));
            expectedT+=duration; boundaries.append(QPointF(expectedT,b));
            expectedT+=pause; boundaries.append(QPointF(expectedT,b));
            expectedT+=duration; boundaries.append(QPointF(expectedT,a));
        }
        const auto now=makeRefTrajectory(c.a,c.b,c.seconds,c.pause,c.n,&newT);
        CHECK(expectedT==newT && now.first()==boundaries.first() && now.last()==boundaries.last());
        CHECK(now.size()==1+cycles*402);
        int k=0;
        for(const auto &boundary:boundaries) {
            while(k<now.size() && now[k]!=boundary) ++k;
            CHECK(k<now.size());
        }
        for(int i=0;i<now.size();++i) {
            CHECK(qIsFinite(now[i].x()) && qIsFinite(now[i].y()));
            CHECK(now[i].y()>=qMin(a,b)-1e-12 && now[i].y()<=qMax(a,b)+1e-12);
            if(i) CHECK(now[i].x()>=now[i-1].x());
        }
        for(int j=1;j<boundaries.size();++j) {
            const auto from=boundaries[j-1],to=boundaries[j];
            if(to.x()<=from.x()) continue;
            qreal previous=from.y();
            for(int sample=0;sample<=401;++sample) {
                const long double u=static_cast<long double>(sample)/401;
                const long double f=6*std::pow(u,5)-15*std::pow(u,4)+10*std::pow(u,3);
                const qreal t=from.x()+(to.x()-from.x())*static_cast<qreal>(u);
                const qreal expected=from.y()+(to.y()-from.y())*static_cast<qreal>(f);
                const qreal position=refValueAt(now,t);
                CHECK(near(position,expected,qMax(1e-8,std::abs(to.y()-from.y())*2e-5)));
                if(to.y()>from.y()) CHECK(position>=previous-1e-9);
                if(to.y()<from.y()) CHECK(position<=previous+1e-9);
                previous=position;
            }
        }
        CHECK(refValueAt(now,-1)==a && refValueAt(now,newT+1)==a);
    }
    qreal t,oldT;
    const auto p=makeRefTrajectory(-30,30,6,2,5,&t);
    legacyMakeRefTrajectory(-30,30,30,2,5,&oldT);
    CHECK(oldT==40 && t==80);
    CHECK(refValueAt(p,2)==-30 && refValueAt(p,8)==30);
    CHECK(refValueAt(p,10)==30 && refValueAt(p,16)==-30);
    CHECK(near(refValueAt(p,3.5),-23.7890625));
    CHECK(near(refValueAt(p,6.5),23.7890625));
    for(double u:{0.,1.}) {
        CHECK(30*u*u*(1-u)*(1-u)==0);
        CHECK(60*u*(1-u)*(1-2*u)==0);
    }
    const double newPeak=60./6*1.875,oldPeak=60./2*1.875;
    CHECK(near(newPeak,18.75) && near(newPeak*3,oldPeak));
    std::printf("PASS S samples, range, reversal, exact requested phase times; default 80 seconds and 18.75 deg/s reference peak\n");
}

static void configure(MainWindow &w,const QStringList &m1,const QStringList &m2,int n,qreal duration1=6,qreal duration2=6) {
    for(int i=0;i<4;++i) { w.m1ActEdits[i]->setText(m1[i]); w.m2ActEdits[i]->setText(m2[i]); }
    w.trackerCyclesSpin->setValue(n);
    w.trackerM1MoveSpin->setValue(duration1); w.trackerM2MoveSpin->setValue(duration2);
}
static void deliver(ProtocolWorker &receiver,const QMap<uint8_t,float> &values) {
    QByteArray payload; payload.append(char(values.size()));
    for(auto it=values.cbegin();it!=values.cend();++it) {
        payload.append(char(it.key())); uint32_t bits;
        const float value=it.value(); std::memcpy(&bits,&value,4);
        for(unsigned b=0;b<4;++b) payload.append(char((bits>>(8*b))&255));
    }
    const auto frame=receiver.buildFrame(CMD_TELEMETRY,payload);
    CHECK(receiver.parseFrame(reinterpret_cast<const uint8_t*>(frame.constData()),frame.size())==frame.size());
}
static void legend(MainWindow &w,MainWindow::TrackerCtx &c) {
    w.refreshTrackerLegend(c);
    for(auto *marker:c.chart->legend()->markers()) {
        const auto *s=marker->series();
        CHECK(marker->isVisible()==(s==c.refSeries || s==c.histTargetSeries || s==c.histActualSeries));
    }
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    const int font=QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    CHECK(font>=0);
    app.setFont(QFont(QFontDatabase::applicationFontFamilies(font).first(),9));
    referenceMath();
    MainWindow w;
    ProtocolWorker receiver;
    unsigned sent=0,received=0;
    QObject::connect(&receiver,&ProtocolWorker::rawDataSent,[&](const QByteArray&){++sent;});
    QObject::connect(w.protoWorker,&ProtocolWorker::rawDataSent,[&](const QByteArray&){++sent;});
    QObject::connect(&receiver,&ProtocolWorker::telemetryReceived,&w,[&](const QMap<uint8_t,float>&values) {
        ++received; w.onTelemetryReceived(values);
    });
    w.showRehabPage();
    w.resize(1500,1100); w.show(); app.processEvents();
    CHECK(w.trackerM1.totalT==80 && w.trackerM2.totalT==80);
    CHECK(w.trackerM1.refSeries->points()==w.trackerM1.refTraj);
    CHECK(w.trackerM2.refSeries->points()==w.trackerM2.refTraj);
    CHECK(w.grab().save("preview-before.png"));
    legend(w,w.trackerM1); legend(w,w.trackerM2);
    const auto initial=w.trackerM1.refTraj;
    CHECK(w.trackerM1MoveSpin->value()==6 && w.trackerM2MoveSpin->value()==6);
    w.trackerM1MoveSpin->setValue(10);
    CHECK(w.trackerM1.totalT==120 && w.trackerM2.totalT==80);
    w.trackerM1MoveSpin->setValue(6);
    CHECK(w.trackerM1.refTraj==initial);
    w.startExperiment(); waitEvents(110);
    CHECK(w.trackerM1.running && w.trackerM2.running);
    CHECK(w.trackerM1.nowT>=.03 && w.trackerM1.nowT<=.5);
    CHECK(std::abs(w.trackerM1.nowT-w.trackerM2.nowT)<=.005);
    CHECK(w.trackerM1.refTraj==initial);
    for(auto *c:{&w.trackerM1,&w.trackerM2}) {
        CHECK(std::abs(c->nowT-c->clock.elapsed()/1000.)<.1);
        CHECK(c->cursorSeries->points().first().x()==c->nowT);
    }
    // All edits are pending while a session exists; even explicit preview cannot
    // mutate active curves, axes, clock epochs, angles, holds or cycle count.
    const auto epoch1=w.trackerM1.clock.msecsSinceReference();
    const auto epoch2=w.trackerM2.clock.msecsSinceReference();
    configure(w,{"900","-20","25","3"},{"900","-12","23","2"},3,9,12);
    w.applyReference(false);
    for(auto *button:w.findChildren<QPushButton*>())
        if(button->text()==QStringLiteral("刷新预览")) button->click();
    CHECK(w.trackerM1.refTraj==initial && w.trackerM2.refTraj==initial);
    CHECK(w.trackerM1.totalT==80 && w.trackerM2.totalT==80);
    CHECK(w.trackerM1.moveSeconds==6 && w.trackerM2.moveSeconds==6);
    CHECK(w.trackerM1.limit1==-30 && w.trackerM1.limit2==30 && w.trackerM1.pause==2 && w.trackerM1.cycles==5);
    CHECK(w.trackerM1.clock.msecsSinceReference()==epoch1 && w.trackerM2.clock.msecsSinceReference()==epoch2);
    w.startExperiment();
    CHECK(w.trackerM1.moveSeconds==9 && w.trackerM2.moveSeconds==12);
    CHECK(w.trackerM1.totalT==72 && w.trackerM2.totalT==84);
    CHECK(w.trackerM1.limit1==-20 && w.trackerM1.limit2==25 && w.trackerM1.pause==3 && w.trackerM1.cycles==3);
    configure(w,{"30","-30","30","2"},{"30","-30","30","2"},5);
    w.startExperiment();
    CHECK(w.trackerM1.refTraj==initial && w.trackerM2.refTraj==initial);
    // Exercise a whole changing telemetry stream through the original parser.
    // Draw deterministic synthetic history; don't wait a real 40-second session.
    w.mTrackerTimer->stop();
    w.m_recording=true;
    w.trackerM1.rawCheck->setChecked(true); w.trackerM2.rawCheck->setChecked(true);
    for(int i=0;i<160;++i) {
        const float a=-25.f+i*.25f,b=17.f-i*.125f;
        const QMap<uint8_t,float> values={{0x01,a+1},{0x02,a},{0x0B,11.f},
            {0x11,b+2},{0x12,b},{0x1D,-12.f},{0x41,2.5f},{0x44,-.75f}};
        w.trackerM1.nowT=w.trackerM2.nowT=i*.1;
        deliver(receiver,values);
        CHECK(w.trackerM1.lastActual==a && w.trackerM2.lastActual==b);
        CHECK(w.trackerM1.lastTarget==a+1 && w.trackerM2.lastTarget==b+2);
        CHECK(w.trackerM1.histActualSeries->points().last()==QPointF(i*.1,a));
        CHECK(w.trackerM2.histActualSeries->points().last()==QPointF(i*.1,b));
        CHECK(w.trackerM1.histTargetSeries->points().last().y()==a+1);
        CHECK(w.trackerM2.histTargetSeries->points().last().y()==b+2);
        CHECK(w.trackerM1.lastRaw==11 && w.trackerM2.lastRaw==-12);
        CHECK(w.m_recordBuffer.last().values==values);
        CHECK(w.trackerM1.refTraj==initial);
    }
    CHECK(w.trackerM1.histActualSeries->count()==160);
    CHECK(w.trackerM2.histActualSeries->count()==160);
    CHECK(w.trackerM1.rawSeries->count()==160 && w.trackerM2.rawSeries->count()==160);
    // Existing invalid feedback filter remains a filter, never a smoothing path.
    const auto actual=w.trackerM1.lastActual;
    const int count=w.trackerM1.histActualSeries->count();
    deliver(receiver,{{0x02,100000.f}});
    CHECK(w.trackerM1.lastActual==actual && w.trackerM1.histActualSeries->count()==count);
    CHECK(w.trackerM1.dropped==1 && w.m_recordBuffer.last().values.value(0x02)==100000.f);
    w.m_recording=false;
    w.trackerM1.running=w.trackerM2.running=false;
    w.updateTrackerViews();
    CHECK(w.grab().save("preview-synthetic-feedback.png"));
    // Independent end times through the actual 33ms timer. Preserve both channels
    // while one is finished and the other is still running.
    configure(w,{"6000","-30","30","0"},{"3000","-15","15","0"},1,1,1.5);
    w.startExperiment(); w.mTrackerTimer->start(); waitEvents(2200);
    CHECK(w.trackerM1.totalT==2 && w.trackerM2.totalT==3);
    CHECK(w.trackerM1.finished && !w.trackerM1.running);
    CHECK(w.trackerM2.running && !w.trackerM2.finished);
    const auto finished1=w.trackerM1.refTraj,stillRunning2=w.trackerM2.refTraj;
    w.trackerM1MoveSpin->setValue(7); w.trackerM2MoveSpin->setValue(8);
    w.trackerCyclesSpin->setValue(5); w.applyReference(false);
    CHECK(w.trackerM1.refTraj==finished1 && w.trackerM2.refTraj==stillRunning2);
    CHECK(w.trackerM1.totalT==2 && w.trackerM2.totalT==3);
    waitEvents(1050);
    CHECK(w.trackerM1.finished && w.trackerM2.finished);
    CHECK(!w.trackerM1.running && !w.trackerM2.running);
    CHECK(w.trackerM1.nowT==2 && w.trackerM2.nowT==3);
    CHECK(w.trackerM1.histActualSeries->count()==0 && w.trackerM2.histActualSeries->count()==0);
    CHECK(w.trackerM1.cursorSeries->points().first().x()==2);
    legend(w,w.trackerM1); legend(w,w.trackerM2);
    w.applyReference(false);
    CHECK(w.trackerM1.refTraj==finished1 && w.trackerM2.refTraj==stillRunning2);
    configure(w,{"999","-30","30","2"},{"999","-12","23","2"},5,6,9);
    CHECK(w.trackerM1.totalT==2 && w.trackerM2.totalT==3);
    w.startExperiment();
    CHECK(!w.trackerM1.finished && !w.trackerM2.finished);
    CHECK(w.trackerM1.running && w.trackerM2.running);
    CHECK(w.trackerM1.refTraj==initial && w.trackerM1.totalT==80);
    CHECK(w.trackerM2.totalT==110 && w.trackerM2.moveSeconds==9);
    CHECK(w.trackerM1.axisX->max()==80 && w.trackerM2.axisX->max()==110);
    CHECK(w.trackerM1.clock.elapsed()<200 && w.trackerM2.clock.elapsed()<200);
    CHECK(!w.trackerM1.hasActual && !w.trackerM2.hasActual);
    CHECK(sent==0 && received==161 && !receiver.isOpen() && !w.protoWorker->isOpen());
    legend(w,w.trackerM1); legend(w,w.trackerM2);
    std::printf("PASS %u checks; independent timing, frozen settings, original parser/feedback/recording; %u synthetic telemetry frames; zero sends; no open ports\n",checks,received);
}
