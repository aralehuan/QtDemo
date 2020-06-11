#ifndef TIMESHARECHART_H
#define TIMESHARECHART_H
#include "qcustomplot.h"
#include "stock/Stock.h"

class TimeShareChart : public QCustomPlot
{
public:
    explicit TimeShareChart(QWidget *parent = 0);

    void setStockKData(Stock* stock, KData* k);
protected:
    int mW;
    int mH;
    double mHighVol;
    double mVols[9];//largeBuy,largeSell,bigBuy,bigSell,midBuy,midSell,SmallBuy,smallSell,buyOrSell
    bool mMark;
    Stock* mStock;
    KData* mK;
    QCPAxisRect *subRectRight;
    QBrush brushs[9];
    QCPItemText* mTraceLB;
    QCPItemText* mVolLB;
    QCPItemStraightLine * mXTrace;
    QCPItemStraightLine * mYTrace;
    void drawTData();
   // bool event(QEvent *event);//不要重写覆盖了QCustomPlot的event,会导致画布大小不对
    void paintEvent(QPaintEvent * event);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void updateMarkPos(double x, double y);
};

#endif // TIMESHARECHART_H
