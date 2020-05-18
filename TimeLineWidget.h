#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QTimer>
#include "stock/StockMgr.h"

class TimeLineWidget: public QWidget
{
     Q_OBJECT
protected:
    const int T0 = (9*3600+15*60)*1000;//集合竞价1开始时间(ms)
    const int L0 = (15*60)*1000;//集合竞价时长
    const int T1 = (9*3600+30*60)*1000;//连续竞价1开始时间
    const int L1 = (2*3600)*1000;//连续竞价时长
    const int T2 = (13*3600)*1000;//连续竞价2开始时间
    const int L2 = (2*3600)*1000;//连续竞价时长
    const float W0=0.2f;

    int        mW;    //图宽
    int        mH;     //图高
    double mHigh;//区间最高价
    double mLow; //区间最低价
    QTimer mTimer;
    //准星相关
    bool mMark;
    int mMarkX;
    int mMarkY;
    //缩放滑动
    float mSX;
public:
     explicit TimeLineWidget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent * event);
    bool event(QEvent *event);
    void drawBack(QPainter& painter);
    void drawT0Back(QPainter& painter);
    void drawT1Back(QPainter& painter);
    void drawTimeLine(QPainter& painter);
    void drawMark(QPainter& painter);

    int time2X(int ms);
    int x2Time(int x);
    int  getCurDayMs();
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
public slots:
    void onTimerRefresh();
};

#endif // TIMELINEWIDGET_H
