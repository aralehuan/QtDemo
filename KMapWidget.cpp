#pragma warning(disable:4819)
#include "KMapWidget.h"
#include <QPainter>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QVector>

KMapWidget::KMapWidget(QWidget *parent) : QWidget(parent)
  ,mKW(9)
  ,mStock(nullptr)
  ,mMark(false)
  ,mPressX(0)
  ,mScrollX(0)
  ,mLastScrollX (0)
{
}

void KMapWidget::setStock(Stock* stock)
{
    if(stock!=mStock)
    {
       mLastScrollX =  mScrollX = mPressX  = 0;
    }
    mStock = stock;
    update();
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
            int h = re->size().height();
            mH2 = (h-24)/3;
            mH = 2*mH2;
            adjustScroll();
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
    if(mStock==nullptr)return;
    const QList<KData*>& history = mStock->getValidHistory();
    if(history.isEmpty())return;
    painter.setBrush(Qt::SolidPattern);
    //从右往左绘制k节点，mStock最前面的为最新记录
    int days = mW / mKW;//最多显示多少天
    int count = history.length();
    int rightK=  mScrollX/mKW;
    int leftK  =  rightK + days;
    if(leftK>count)leftK=count;
     //qDebug()<<mScrollX<<","<<rightK<<","<<leftK<<","<<count<<","<<days;
    //计算区间最低最高价
    mHigh = 0;
    mLow  = 10000;
    mHighVol = 0;
    for(int i=rightK;i<leftK;++i)
    {
        KData* k = history[i];
        if(mHigh<k->high)mHigh=k->high;
        if(mLow>k->low)mLow=k->low;
        if(mHighVol<k->volume)mHighVol=k->volume;
    }
    drawKNode(painter, rightK, leftK);
    drawMark(painter, rightK, leftK);
    //标线价
    painter.setFont(QFont(QString("Times New Roman"), 5));
    painter.setPen(Qt::black);
    int fontHight = painter.fontMetrics().height();
    painter.drawText(0,  fontHight, QString::number(mHigh,'f',2));
    painter.drawText(0,  0.25f*mH, QString::number(mLow+0.75f*(mHigh-mLow),'f',2));
    painter.drawText(0,  0.50f*mH, QString::number(mLow+0.50f*(mHigh-mLow),'f',2));
    painter.drawText(0,  0.75f*mH, QString::number(mLow+0.25f*(mHigh-mLow),'f',2));
    painter.drawText(0,  mH, QString::number(mLow,'f',2));
    //量图
    drawVolume(painter, rightK, leftK);
}

 void KMapWidget::drawKNode(QPainter& painter, int rightK, int leftK)
 {//从右向左绘制
     float x = (leftK-rightK-1)*mKW;
     float halfKW = mKW/2;
     float pixel = 1.0f*mH/(mHigh-mLow);
     QVector<QLineF> lines;
     const QList<KData*>& history = mStock->getValidHistory();
     for(int i=rightK;i<leftK;++i)
     {
        KData* k = history[i];
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
        //k节点
        painter.drawLine(x+halfKW,(mHigh - k->high)*pixel, x+halfKW, (mHigh - k->low)*pixel);
        painter.drawRect(x+1,(mHigh - up)*pixel, mKW-2, abs(k->close-k->open)*pixel);
        //当日均价线
        painter.setPen(Qt::black);
        painter.drawLine(x+halfKW-1,(mHigh - k->avg)*pixel, x+halfKW+1, (mHigh -k->avg)*pixel);
        //计算5日均线坐标
        float nextma = i+1<history.count()? history[i+1]->ma5:k->ma5;
        if(nextma==0)nextma = k->ma5;
        lines.append(QLineF(x+halfKW, (mHigh - k->ma5)*pixel, x-halfKW,   (mHigh -nextma)*pixel));
        x-=mKW;
     }
     //绘制均线
     QPen pen(Qt::SolidLine);
     pen.setWidthF(0.5f);
     pen.setColor(Qt::blue);
     pen.setJoinStyle(Qt::RoundJoin);
     painter.setPen(pen);
     painter.setRenderHint(QPainter::Antialiasing);
     painter.drawLines(lines);;
     painter.setRenderHint(QPainter::Antialiasing,false);
 }

void KMapWidget::drawBack(QPainter& painter)
{
    painter.setPen(QColor(240,240,240));
    painter.drawRect(0,0,mW-1,mH-1);
    painter.drawLine(0,0.25f*mH, mW, 0.25f*mH);
    painter.drawLine(0,0.5f*mH, mW, 0.5f*mH);
    painter.drawLine(0,0.75f*mH, mW, 0.75f*mH);
}

void KMapWidget::drawMark(QPainter& painter, int rightK, int leftK)
{
    if(!mMark)return;
    if(mMarkX<mKW/2)mMarkX=mKW/2;
    if(mMarkX>(leftK-rightK)*mKW-mKW/2)mMarkX=(leftK-rightK)*mKW-mKW/2;
    if(mMarkY<0)mMarkY=0;
     if(mMarkY>mH)mMarkY=mH;

    painter.setPen(Qt::black);
    painter.drawLine(0,mMarkY,mW,mMarkY);
    painter.drawLine(mMarkX,0,mMarkX,mH);
    float price = mLow + 1.0f*(mH-mMarkY)/mH*(mHigh-mLow);
     int fontHight = painter.fontMetrics().height();
     float priceY = mMarkY-fontHight/2;
     if(priceY<0)priceY=0;
     if(priceY>mH-fontHight)priceY=mH-fontHight;
     painter.setBrush(Qt::black);
    painter.drawRect(0,priceY,36,fontHight);
    painter.setPen(Qt::red);
    painter.drawText(0,priceY+fontHight, QString::number(price,'f',2));

    int idx = leftK-mMarkX/mKW-1;
    KData* k = idx<0?nullptr:mStock->getValidHistory()[idx];
    emit selectK(k);
}

void KMapWidget::drawVolume(QPainter& painter, int rightK, int leftK)
{
    //背景框
    painter.translate(0, mH+24);
    painter.setPen(QColor(240,240,240));
    painter.setBrush(Qt::white);
    painter.drawRect(0,0,mW-1,mH2-1);

    float x = (leftK-rightK-1)*mKW;
    float halfKW = mKW/2;
    float pixel = 1.0f*mH2/mHighVol;
    QVector<QLineF> lines;
    const QList<KData*>& history = mStock->getValidHistory();
    for(int i=rightK;i<leftK;++i)
    {
       KData* k = history[i];
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

       painter.drawRect(x+1,mH2-k->volume*pixel, mKW-2, k->volume*pixel);
       x-=mKW;
    }
    //标线
    if(mMark)
    {
        painter.setPen(Qt::black);
        painter.drawLine(mMarkX,0,mMarkX,mH2);
    }
    //最高量
    painter.setFont(QFont(QString("Times New Roman"), 5));
    painter.setPen(Qt::black);
    int fontHight = painter.fontMetrics().height();
    painter.drawText(0,  fontHight, QString::number(mHighVol,'f',2));
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
        mLastScrollX = mScrollX;
        mPressX = e->x();
        this->update();
    }
}

void KMapWidget::mouseReleaseEvent(QMouseEvent *e)
{
    mMark = false;
    this->update();
}

void KMapWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(mMark)
    {
        mMarkX = e->x()/mKW*mKW+mKW/2;
        mMarkY = e->y();
        this->update();
    }
    else
    {
        mScrollX = mLastScrollX+e->x()-mPressX;
        adjustScroll();
        this->update();
    }
}

void KMapWidget::adjustScroll()
{
    if(mStock==nullptr)return;
    int count =  mStock->getValidHistory().length();
    float contentSize = count*mKW;
    if(mScrollX>(contentSize - mW+mKW))
    {
        //qDebug()<<"pull";拉取更老数据
        mScrollX=contentSize - mW+mKW;
    }
    if(mScrollX<0)mScrollX=0;
}
