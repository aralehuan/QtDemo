#ifndef KMAPWIDGET_H
#define KMAPWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QTimer>
#include "AraleStock.h"

class KMapWidget : public QWidget
{
    Q_OBJECT
private:
    Stock* mStock;
    int mW;  //k图宽
    int mH;   //k图高
    int mKW;//k节点宽度
    float mHigh;//区间最高价
    float mLow;//区间最低价
    double mHighVol;//区间最高量

    //准星相关
    bool mMark;
    int mMarkX;
    int mMarkY;

    //滑动相关
    int mPressX;
    int mScrollX;
    int mLastScrollX;

    //交易量图
    int mH2;//量图高
public:
    explicit KMapWidget(QWidget *parent = 0);
    void setStock(Stock* stock);

protected:
    void paintEvent(QPaintEvent * event);
    bool event(QEvent *event);
    void drawKNode(QPainter& painter, int rightK, int leftK);
    void drawBack(QPainter& painter);
    void drawMark(QPainter& painter, int rightK, int leftK);
    void drawVolume(QPainter& painter, int rightK, int leftK);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void adjustScroll();
signals:
    void selectK(KData* k);//无需实现
};

#endif // KMAPWIDGET_H
