#pragma warning(disable:4819)
#include "KMapWidget.h"
#include <QPainter>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QVector>
#include "qcustomplot.h"

KMapWidget::KMapWidget(QWidget *parent) : QWidget(parent)
  ,mKW(9)
  ,mStock(nullptr)
  ,mSelK(nullptr)
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
    const QList<KData*>& history = mStock->getValidHistory();
    emit selectK(history.isEmpty()?nullptr:mSelK=history[0]);
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
    QCPPainter painter(this);
    //QPainter painter(this);
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
     QVector<QLineF> lines5;
     QVector<QLineF> lines10;
     QVector<QLineF> lines20;
     const QList<KData*>& history = mStock->getValidHistory();
     for(int i=rightK;i<leftK;++i)
     {
        KData* k = history[i];
        //选中的k背景
        if(!mMark && k==mSelK)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(240,240,240));
            painter.drawRect(x+1,0, mKW-2, mH);
        }
         //绘制k节点
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
        painter.drawLine(x+halfKW,(mHigh - k->high)*pixel, x+halfKW, (mHigh - k->low)*pixel);
        painter.drawRect(x+1,(mHigh - up)*pixel, mKW-2, abs(k->close-k->open)*pixel);
        //当日均价标记
        painter.setPen(Qt::black);
        painter.drawLine(x+halfKW-1,(mHigh - k->avg)*pixel, x+halfKW+1, (mHigh -k->avg)*pixel);
        //计算日均线坐标
        float nextma5 = i+1<history.count()? history[i+1]->ma5:k->ma5;
        float nextma10 = i+1<history.count()? history[i+1]->ma10:k->ma10;
        float nextma20 = i+1<history.count()? history[i+1]->ma20:k->ma20;
        if(nextma5==0)nextma5 = k->ma5;
        if(nextma10==0)nextma10 = k->ma10;
        if(nextma20==0)nextma20 = k->ma20;
        lines5.append(QLineF(x+halfKW, (mHigh - k->ma5)*pixel, x-halfKW,   (mHigh -nextma5)*pixel));
        lines10.append(QLineF(x+halfKW, (mHigh - k->ma10)*pixel, x-halfKW,   (mHigh -nextma10)*pixel));
        lines20.append(QLineF(x+halfKW, (mHigh - k->ma20)*pixel, x-halfKW,   (mHigh -nextma20)*pixel));
        x-=mKW;
     }

     //绘制均线
     QPen pen(Qt::SolidLine);
     pen.setWidthF(0.5f);
     pen.setJoinStyle(Qt::RoundJoin);
     painter.setPen(pen);
     painter.setRenderHint(QPainter::Antialiasing);
     painter.setPen(Qt::blue);
     painter.drawLines(lines5);
     painter.setPen(Qt::cyan);
     painter.drawLines(lines10);
     painter.setPen(Qt::magenta);
     painter.drawLines(lines20);
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
    if(k!=nullptr)
    {
        painter.setPen(Qt::blue);
        painter.drawText(0,mH+fontHight, QString().sprintf("MA5:%0.2f", k->ma5));
        painter.drawText(0,mH+24, QString().sprintf("VMA5:%0.0f", k->vma5));
        painter.setPen(Qt::cyan);
        painter.drawText(120,mH+fontHight, QString().sprintf("MA10:%0.2f", k->ma10));
        painter.drawText(120,mH+24, QString().sprintf("VMA10:%0.0f", k->vma10));
        painter.setPen(Qt::magenta);
        painter.drawText(240,mH+fontHight, QString().sprintf("MA20:%0.2f", k->ma20));
    }
    emit selectK(mSelK=k);
}

void KMapWidget::drawVolume(QPainter& painter, int rightK, int leftK)
{
    //背景框
    painter.translate(0, mH+24);
    painter.setPen(QColor(240,240,240));
    painter.setBrush(Qt::white);
    painter.drawRect(0,0,mW-1,mH2-1);

    QVector<QLineF> lines5;
    QVector<QLineF> lines10;
    float x = (leftK-rightK-1)*mKW;
    float halfKW = mKW/2;
    float pixel = 1.0f*mH2/mHighVol;
    QVector<QLineF> lines;
    const QList<KData*>& history = mStock->getValidHistory();
    for(int i=rightK;i<leftK;++i)
    {
       KData* k = history[i];
       //选中的k背景
       if(!mMark && k==mSelK)
       {
           painter.setPen(Qt::NoPen);
           painter.setBrush(QColor(240,240,240));
           painter.drawRect(x+1,0, mKW-2, mH2);
       }
       //量节点
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
       //计算日均量坐标
       float nextvma5 = i+1<history.count()? history[i+1]->vma5:k->vma5;
       float nextvma10 = i+1<history.count()? history[i+1]->vma10:k->vma10;
       if(nextvma5==0)nextvma5 = k->vma5;
       if(nextvma10==0)nextvma10 = k->vma10;
       lines5.append(QLineF(x+halfKW, ( mHighVol- k->vma5)*pixel, x-halfKW,   (mHighVol -nextvma5)*pixel));
       lines10.append(QLineF(x+halfKW, (mHighVol - k->vma10)*pixel, x-halfKW,   (mHighVol -nextvma10)*pixel));
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

    //绘制均线
    QPen pen(Qt::SolidLine);
    pen.setWidthF(0.5f);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::blue);
    painter.drawLines(lines5);
    painter.setPen(Qt::cyan);
    painter.drawLines(lines10);
    painter.setRenderHint(QPainter::Antialiasing,false);
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
