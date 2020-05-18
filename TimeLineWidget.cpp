#include "TimeLineWidget.h"
#include <QPainter>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QVector>
#include <chrono>
#include <ctime>
//#include "StockMgr.h"

TimeLineWidget::TimeLineWidget(QWidget *parent)
    : QWidget(parent)
    ,mMark(false)
    ,mSX(1.0f)
{
    connect(&mTimer, SIGNAL(timeout()), this, SLOT(onTimerRefresh()));
}

void TimeLineWidget::onTimerRefresh()
{
    this->update();
}


bool TimeLineWidget::event(QEvent *event)
{
    QEvent::Type et = event->type();
    switch (et)
    {
        case QEvent::Resize:
        {
            QResizeEvent* re = static_cast<QResizeEvent*>(event);
            mW = re->size().width();
            mH = re->size().height()-12;
            return true;
        }
    case QEvent::Show:
        mTimer.start(100);
        return QWidget::event(event);
    case  QEvent::Hide:
        mTimer.stop();
        return QWidget::event(event);
    default:
        return QWidget::event(event);
    }
}

void TimeLineWidget::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    drawBack(painter);
    drawTimeLine(painter);
}

void TimeLineWidget::drawBack(QPainter& painter)
{
    painter.setPen(QColor(240,240,240));
    painter.drawRect(0,0,mW-1,mH-1);
    drawT0Back(painter);
    drawT1Back(painter);
    //价格分线
    painter.resetTransform();
    painter.drawLine(0,0.25f*mH, mW, 0.25f*mH);
    painter.drawLine(0,0.5f*mH, mW, 0.5f*mH);
    painter.drawLine(0,0.75f*mH, mW, 0.75f*mH);
    drawMark(painter);
}

void TimeLineWidget::drawT0Back(QPainter& painter)
{
    painter.setBrush(Qt::SolidPattern);
    painter.setBrush(QColor(250,250,250));
    float w0 = W0*mW;
    painter.drawRect(0,0,w0,mH-1);
    painter.drawLine(w0/2,0,w0/2,mH);
}

void TimeLineWidget::drawT1Back(QPainter& painter)
{
    painter.translate(W0*mW,0);
    float w = mW-W0*mW;
    painter.drawLine(0.25f*w,0,0.25f*w,mH);
    painter.drawLine(0.50f*w,0,0.50f*w,mH);
    painter.drawLine(0.75f*w,0,0.75f*w,mH);
}

void TimeLineWidget::drawTimeLine(QPainter& painter)
{
    painter.setPen(Qt::blue);
    /*QVector<QLineF> lines;
    MStock* s = StockMgr::GetStock("300264");
    mHigh = s->mHighLimit;
    mLow   = s->mLowLimit;
    float pixel = 1.0f*mH/(mHigh-mLow);
    QuotationsList& qs = s->mQuotations;
    for(int i=0,count=qs.size();i<count;++i)
    {
        QuotationNode& q = qs[i];
        float x = time2X(q.ms100);
        float y = (mHigh - q.price)*pixel;
        float x2=x;
        float y2=y;
        if(i+1<count)
        {
            x2 = time2X(qs[i+1].ms100);
            y2 = (mHigh - qs[i+1].price)*pixel;
        }
         lines.append(QLineF(x, y, x2, y2));
    }
    painter.drawLines(lines);
    //当前时间刻度
    float nx = time2X(getCurDayMs());
    if(nx<0)nx=0;
    if(nx>mW-1)nx=mW-1;
    painter.setPen(Qt::red);
    painter.drawLine(nx,0,nx,mH);
    //标线价
    painter.setFont(QFont(QString("Times New Roman"), 5));
    painter.setPen(Qt::black);
    int fontHight = painter.fontMetrics().height();
    painter.drawText(0,  fontHight, QString::number(mHigh,'f',2));
    painter.drawText(0,  0.25f*mH, QString::number(mLow+0.75f*(mHigh-mLow),'f',2));
    painter.drawText(0,  0.50f*mH, QString::number(mLow+0.50f*(mHigh-mLow),'f',2));
    painter.drawText(0,  0.75f*mH, QString::number(mLow+0.25f*(mHigh-mLow),'f',2));
    painter.drawText(0,  mH, QString::number(mLow,'f',2));*/
}

void TimeLineWidget::drawMark(QPainter& painter)
{
    if(!mMark)return;
    if(mMarkX<0)mMarkX=0;
    if(mMarkX>mW-1)mMarkX=mW-1;
    if(mMarkY<0)mMarkY=0;
    if(mMarkY>mH-1)mMarkY=mH-1;
    painter.setPen(Qt::black);
    painter.drawLine(mMarkX,0,mMarkX,mH);
    painter.drawLine(0,mMarkY,mW,mMarkY);
    int fontHight = painter.fontMetrics().height();
    //横向时间
    char buf[32];
    int ms = x2Time(mMarkX);
    int h = ms/3600000;
    int m = ms%3600000/60000;
    int s = ms%60000/1000;
    sprintf(buf,"%02d:%02d:%02d %03d", h,m,s,ms%1000);
    int tw = painter.fontMetrics().width(buf);
    painter.drawText(mMarkX<mW-tw?mMarkX:mW-tw,  mH+fontHight, buf);
    //纵向价格
    double price = mLow + 1.0f*(mH-mMarkY)/mH*(mHigh-mLow);
    sprintf(buf,"%0.2f", price);
    float priceY = mMarkY-fontHight/2;
    if(priceY<0)priceY=0;
    if(priceY>mH-fontHight)priceY=mH-fontHight;
    painter.setBrush(Qt::black);
    tw = painter.fontMetrics().width(buf);
    painter.drawRect(0,priceY,tw,fontHight);
    painter.setPen(Qt::red);
    painter.drawText(0,priceY+fontHight, buf);
}

int  TimeLineWidget::getCurDayMs()
{//获取相对今天00:00:00时间的毫秒数
    auto now = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&t);
    tm->tm_hour=0;
    tm->tm_min=0;
    tm->tm_sec=0;
    time_t starttm = std::mktime(tm);
    std::chrono::system_clock::time_point starttp = std::chrono::system_clock::from_time_t(starttm);
    return std::chrono::duration_cast<std::chrono::milliseconds>(now-starttp).count();
}

int TimeLineWidget::time2X(int ms)
{
    float w0 = W0*mW;
    float w1 = (mW-w0)/2;
    if(ms<T0)return 0;
    else if(ms<T1)return 1.0f*(ms-T0)/L0*w0;
    else if(ms<T2)return  w0+1.0f*(ms-T1)/L1*w1;
    else return w0+w1+1.0f*(ms-T2)/L2*w1;
}

int TimeLineWidget::x2Time(int x)
{
    float w0 = W0*mW;
    float w1 = (mW-w0)/2;
    if(x<w0)return T0+1.0f*x/w0*L0;
    else if(x<w0+w1)return T1+1.0f*(x-w0)/w1*L1;
    else return T2+1.0f*(x-w0-w1)/w1*L2;
}

void TimeLineWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
    {
        mMark=true;
        mMarkX = e->x();
        mMarkY = e->y();
        this->update();
    }
}

void TimeLineWidget::mouseReleaseEvent(QMouseEvent *e)
{
    mMark = false;
    this->update();
}

void TimeLineWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(mMark)
    {
        mMarkX = e->x();
        mMarkY = e->y();
        this->update();
    }
}
