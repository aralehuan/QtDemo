#pragma execution_character_set("utf-8")
#include "TimeShareChart.h"

TimeShareChart::TimeShareChart(QWidget *parent):QCustomPlot(parent)
,mMark(false)
,mStock(nullptr)
,mK(nullptr)
,mHighVol(0)
{
    memset(mVols,0,sizeof (mVols));
    brushs[0] = QBrush(QColor(100, 0, 0));
    brushs[1] = QBrush(QColor(0, 100, 0));
    brushs[2] = QBrush(QColor(150, 0, 0));
    brushs[3] = QBrush(QColor(0, 150, 0));
    brushs[4] = QBrush(QColor(200, 0, 0));
    brushs[5] = QBrush(QColor(0, 200, 0));
    brushs[6] = QBrush(QColor(250, 0, 0));
    brushs[7] = QBrush(QColor(0, 250, 0));
    brushs[8] = QBrush(QColor(0, 0, 0));
    //设置轴标签
    //yAxis->setLabel("量(单位手)");
    //xAxis->setLabel("分笔");
    //设置拖拽，缩放，选择
    setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    //设置背景色
    this->setBackground(Qt::white);
    //多表布局
    //this->plotLayout()->setWrap(1);//让根layout垂直布局
    this->axisRect(0)->setupFullAxesBox(true);
    this->axisRect(0)->setRangeZoomAxes(this->xAxis,nullptr);//只对该坐标系x轴拖拽和缩放
    this->axisRect(0)->setRangeDragAxes(this->xAxis,nullptr);
    subRectRight = new QCPAxisRect(this);
    subRectRight->setRangeDragAxes(nullptr,nullptr);//禁用该坐标系的拖拽和缩放
    subRectRight->setRangeZoomAxes(nullptr,nullptr);
    this->plotLayout()->addElement(subRectRight);
    subRectRight->setMaximumSize(140, 1000);
    subRectRight->setupFullAxesBox(true);
    subRectRight->axis(QCPAxis::AxisType::atRight)->setTicks(false);
    subRectRight->axis(QCPAxis::AxisType::atTop)->setTicks(false);
    QCPAxis* subXAxis = subRectRight->axis(QCPAxis::AxisType::atBottom);
    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    QVector<double> ticks;
    QVector<QString> labels;
    ticks << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9;
    labels << "BL" << "BB" << "BM" << "BS" << "SL" << "SB" << "SM" << "SS" << "T";
    textTicker->addTicks(ticks,labels);
    subXAxis->setTicker(textTicker);
    // 保持一个好的习惯，将它们放置在相应的层
    //QCustomPlot默认分配了6个层
    //背景层: 绘制背景图
    //网格层: 绘制网格线，每一个坐标轴对应一个网格对象
    //主层: 绘制图表
    //坐标轴层: 绘制坐标轴
    //图例层: 绘制图例
    //overlay层: 绘制最上层的东西,鼠标选择矩形框在此层绘制
    QColor clr(240,240,240);
    foreach (auto *rect, axisRects())
    {
            foreach (auto *axis, rect->axes())
            {
                axis->setLayer("axes");
                axis->grid()->setLayer("grid");
                //设置轴和网格颜色
                axis->grid()->setPen(Qt::SolidLine);
                axis->grid()->setPen(clr);
                axis->setBasePen(clr);//轴
                axis->setTickPen(clr);//主刻度
                axis->setSubTickPen(clr);//子刻度
            }
    }

    //标线信息
    mXTrace = new QCPItemStraightLine (this);
    mXTrace->setLayer("overlay");
    mXTrace->setPen(QPen(Qt::black));
    mXTrace->setClipToAxisRect(true);
    mXTrace->setVisible(false);
    mYTrace = new QCPItemStraightLine (this);
    mYTrace->setLayer("overlay");
    mYTrace->setPen(QPen(Qt::black));
    mYTrace->setClipToAxisRect(true);
    mYTrace->setVisible(false);
    mTraceLB = new QCPItemText(this);
    mTraceLB->setLayer("overlay");
    mTraceLB->setClipToAxisRect(false);
    mTraceLB->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
    mTraceLB->setFont(QFont("Arial", 8));
    mTraceLB->setVisible(false);
    mVolLB = new QCPItemText(this);
    mVolLB->setLayer("overlay");
    mVolLB->setClipToAxisRect(false);
    mVolLB->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
    mVolLB->setFont(QFont("Arial", 8));
    mVolLB->setVisible(false);
}

void TimeShareChart::setStockKData(Stock* stock, KData* k)
{
    if(!this->isVisible())return;
    mHighVol=0;
    mStock = stock;
    mK = k;
    memset(mVols,0,sizeof (mVols));
    drawTData();
}

void TimeShareChart::drawTData()
{
    if(this->plottableCount()>0)this->clearPlottables();
    if(this->graphCount()>0)this->clearGraphs();
    const QList<TData>* ls = mStock->getTData(mK->date);
    if(ls==nullptr||ls->empty())
    {
        this->replot(RefreshPriority::rpQueuedReplot);//必须，前面clear了也要调用，否则crash
        return;
    }

    QCPGraph * graph = this->addGraph();
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setBrush(QBrush(QColor(170, 112, 150, 150)));
    QCPBars* buyVol = new QCPBars(this->xAxis,this->yAxis);
    buyVol->setPen(QPen(QColor(255,0,0).lighter(130)));
    QCPBars* sellVol = new QCPBars(this->xAxis,this->yAxis);
    sellVol->setPen(QPen(QColor(0,255,0).lighter(130)));
    QCPBars* borsVol = new QCPBars(this->xAxis,this->yAxis);
    borsVol->setPen(QPen(QColor(0,0,0).lighter(130)));

    int count=ls->size();
    for(int i=0;i<count;++i)
    {
       const TData& t =  ls->value(i);
       if(t.volume>mHighVol)mHighVol=t.volume;
    }
    //刻度范围
    this->xAxis->setRange(0, count);
    this->yAxis->setRange(-mHighVol, mHighVol);
    for(int i=0;i<ls->size();++i)
    {
       const TData& t =  ls->value(i);
       if(t.side>0)
       {
           buyVol->addData(i,t.volume);
           if(t.volume*t.price*100<50000)
           {
               mVols[0]+=t.volume;
           }
           else if(t.volume*t.price*100<200000)
           {
               mVols[2]+=t.volume;
           }
           else if(t.volume*t.price*100<1000000)
           {
               mVols[4]+=t.volume;
           }
           else
           {
               mVols[6]+=t.volume;
           }
       }
       else if(t.side<0)
       {
            sellVol->addData(i,-t.volume);
            if(t.volume*t.price*100<50000)
            {
                mVols[1]+=t.volume;
            }
            else if(t.volume*t.price*100<200000)
            {
                mVols[3]+=t.volume;
            }
            else if(t.volume*t.price*100<1000000)
            {
                mVols[5]+=t.volume;
            }
            else
            {
                mVols[7]+=t.volume;
            }
       }
       else
       {
            borsVol->addData(i,t.volume);
            mVols[8]+=t.volume;
       }

       graph->addData(i, (t.price-mK->low)/(mK->high - mK->low+0.01)*mHighVol);
    }

    //柱状分组图
    QCPAxis* subXAxis = subRectRight->axis(QCPAxis::AxisType::atBottom);
    QCPAxis* subYAxis = subRectRight->axis(QCPAxis::AxisType::atLeft);
    subXAxis->setRange(0,10);
    subYAxis->setRange(0,mK->volume/100);//mK->volume的单位为股
    for(int i=0;i<4;++i)
    {
        QCPBars* bar = new QCPBars(subXAxis,subYAxis);
        bar->setAntialiased(false);
        bar->setPen(Qt::NoPen);
        bar->setBrush(brushs[i*2]);
        bar->addData(i*2+1,mVols[i*2]);

        bar = new QCPBars(subXAxis,subYAxis);
        bar->setAntialiased(false);
        bar->setPen(Qt::NoPen);
        bar->setBrush(brushs[i*2+1]);
        bar->addData(i*2+2,mVols[i*2+1]);
    }

    //柱状堆积图
    int id = 9;
    QCPBars* lastbar = new QCPBars(subXAxis,subYAxis);//让黑条在最下面可以少画一条柱
    lastbar->setPen(Qt::NoPen);
    lastbar->setBrush(brushs[8]);
    lastbar->addData(id,mVols[8]);
    for(int i=0;i<8;++i)
    {
        QCPBars* bar = new QCPBars(subXAxis,subYAxis);
        bar->setPen(Qt::NoPen);
        bar->setBrush(brushs[i]);
        bar->addData(id,mVols[i]);
        bar->moveAbove(lastbar);
        lastbar=bar;
    }

    this->replot(RefreshPriority::rpQueuedReplot);//更新绘图,错误的刷新模式会导致崩溃
}

void TimeShareChart::mousePressEvent(QMouseEvent *e)
{
    QCustomPlot::mousePressEvent(e);
    if((e->button() & Qt::RightButton) && mHighVol>0)
    {
        mMark=true;
        float xval = this->xAxis->pixelToCoord(e->x());
        float yval = this->yAxis->pixelToCoord(e->y());
        updateMarkPos(xval, yval);
    }
    mXTrace->setVisible(mMark);
    mYTrace->setVisible(mMark);
    mTraceLB->setVisible(mMark);
    mVolLB->setVisible(mMark);
    this->replot(RefreshPriority::rpQueuedReplot);
}

void TimeShareChart::mouseReleaseEvent(QMouseEvent *e)
{
    QCustomPlot::mouseReleaseEvent(e);
    mMark = false;
    mXTrace->setVisible(mMark);
    mYTrace->setVisible(mMark);
    mTraceLB->setVisible(mMark);
    mVolLB->setVisible(mMark);
    this->replot(RefreshPriority::rpQueuedReplot);
}

void TimeShareChart::mouseMoveEvent(QMouseEvent *e)
{
    QCustomPlot::mouseMoveEvent(e);
    if(!mMark)return;
    //转换到轴坐标
    float xval = this->xAxis->pixelToCoord(e->x());
    float yval = this->yAxis->pixelToCoord(e->y());
    updateMarkPos(xval, yval);
    this->replot(RefreshPriority::rpQueuedReplot);
}

void TimeShareChart::updateMarkPos(double x, double y)
{
    if(mHighVol<=0)return;
    if(x<xAxis->range().lower)x=xAxis->range().lower;
    if(x>xAxis->range().upper)x=xAxis->range().upper;
    if(y<yAxis->range().lower)y=yAxis->range().lower;
    if(y>yAxis->range().upper)y=yAxis->range().upper;
    mXTrace->point1->setCoords(x, yAxis->range().lower);
    mXTrace->point2->setCoords(x, yAxis->range().upper);
    mYTrace->point1->setCoords(xAxis->range().lower, y);
    mYTrace->point2->setCoords(xAxis->range().upper, y);
    mTraceLB->position->setCoords(x,yAxis->range().upper);
    mVolLB->position->setCoords(xAxis->pixelToCoord(this->axisRect(0)->left()),y);
    const QList<TData>* ls = mK->timeData;
    int index = (int)x;
    if(index>=ls->size())index=ls->size()-1;
    if(index<0)index=0;
    const TData& t =  ls->value(index);
    mTraceLB->setText(QString().sprintf(" 量 %0.2f手 价 %0.2f 时间 %02d:%02d:%02d", t.side*t.volume, t.price, t.time/10000,t.time%10000/100,t.time%100));
    mVolLB->setText(QString::number(y,'f',2)+"手");
}

 void TimeShareChart::paintEvent(QPaintEvent * event)
 {
     QCustomPlot::paintEvent(event);
     if(mK==nullptr)return;
     QPainter painter(this);
     QCPAxis* subXAxis = subRectRight->axis(QCPAxis::AxisType::atBottom);
     QCPAxis* subYAxis = subRectRight->axis(QCPAxis::AxisType::atLeft);
     int halfFontHight = painter.fontMetrics().height()/2;
     painter.setPen(QColor(150,150,150));
     for(int i=0;i<8;++i)
     {
         int x = subXAxis->coordToPixel(i+1)-halfFontHight;
         int y = subYAxis->coordToPixel(mK->volume/100);
         painter.resetTransform();
         painter.translate(x,y);
         painter.rotate(90);
         painter.drawText(0,0,QString().sprintf("%0.2f  %0.2f%",mVols[i],mVols[i]/mK->volume*10000));
     }
 }
