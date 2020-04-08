#include "KMapWidget.h"
#include <QPainter>
#include <QDebug>
#include <QEvent>

KMapWidget::KMapWidget(QWidget *parent) : QWidget(parent),mKW(9),mStock(nullptr),mMark(false)
{
}

bool KMapWidget::event(QEvent *event)
{ 
    QEvent::Type et = event->type();
    switch (et)
    {
        case QEvent::Resize:
        {
            QResizeEvent* re = static_cast<QResizeEvent*>(event);
            mW = re->size().width();
            mH = re->size().height();
            return true;
        }
    default:
        return QWidget::event(event);
    }
}

void KMapWidget::paintEvent(QPaintEvent * event)
{
    QPainter painter(this);
    drawBack(painter);
    if(mStock==nullptr || mStock->history.isEmpty())return;
    painter.setBrush(Qt::SolidPattern);
    int days = mW / mKW;//最多显示多少天/*
    int count = mStock->history.length();
    float x = 0;
    float o = mKW/2;
    float high = 0;
    float low  = 10000;

    for(int i=0,max=days<count?days:count;i<max;++i)
    {//获取区间最低价和最高价
        KData* k = mStock->history[i];
        if(high<k->high)high=k->high;
        if(low>k->low)low=k->low;
    }

    float pixel = 1.0f*mH/(high-low);
    for(int i=0,max=days<count?days:count;i<max;++i)
    {
        KData* k = mStock->history[i];
        float up = k->open>k->close?k->open:k->close;
        if(k->close>k->open)
        {
            painter.setBrush(Qt::white);
            painter.setPen(Qt::red);
        }
        else if(k->close<k->open)
        {
            painter.setBrush(QColor(0,160,0));
            painter.setPen(QColor(0,160,0));
        }
        else
        {
            painter.setBrush(Qt::black);
            painter.setPen(Qt::black);
        }

        painter.drawLine(x+o,(high - k->high)*pixel, x+o, (high - k->low)*pixel);
        painter.drawRect(x,(high - up)*pixel, mKW-1, abs(k->close-k->open)*pixel);
        x+=mKW;
    }
    //painter.drawText(0,24,QString("helloworld"));
    drawMark(painter,high,low);
}

void KMapWidget::drawBack(QPainter& painter)
{
    painter.setPen(QColor(240,240,240));
    painter.drawRect(0,0,mW-1,mH-1);
    painter.drawLine(0,0.25f*mH, mW, 0.25f*mH);
    painter.drawLine(0,0.5f*mH, mW, 0.5f*mH);
    painter.drawLine(0,0.75f*mH, mW, 0.75f*mH);
}

void KMapWidget::drawMark(QPainter& painter, float high, float low)
{
    if(!mMark)return;
    painter.setPen(Qt::black);
    painter.drawLine(0,mMarkY,mW,mMarkY);
    painter.drawLine(mMarkX,0,mMarkX,mH);
    float price = low + 1.0f*(mH-mMarkY)/mH*(high-low);
    painter.setPen(Qt::red);
    painter.drawText(0,mMarkY, QString::number(price,'f',2));


    int count = mStock->history.count();
    int idx = mMarkX/mKW;
    KData* k = idx>=count?nullptr:mStock->history[idx];
    emit selectK(k);
}

void KMapWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->button() & Qt::RightButton)
    {
        mMark=true;
        mMarkX = e->x()/mKW*mKW+mKW/2;
        mMarkY = e->y();
        this->update();
    }
    else if(e->button() & Qt::LeftButton)
    {

    }
}

void KMapWidget::mouseReleaseEvent(QMouseEvent *e)
{
    mMark = false;
    this->update();
}

void KMapWidget::mouseMoveEvent(QMouseEvent *e)
{
    mMarkX = e->x()/mKW*mKW+mKW/2;
    mMarkY = e->y();
    this->update();
}
