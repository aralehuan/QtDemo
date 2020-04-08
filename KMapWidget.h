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
    int mW;
    int mH;
    int mKW;
    Stock* mStock;

    bool mMark;
    int mMarkX;
    int mMarkY;
public:
    explicit KMapWidget(QWidget *parent = 0);
    void setStock(Stock* stock){mStock = stock;update();}

protected:
    void paintEvent(QPaintEvent * event);
    bool event(QEvent *event);
    void drawBack(QPainter& panter);
    void drawMark(QPainter& panter, float high, float low);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
};

#endif // KMAPWIDGET_H
