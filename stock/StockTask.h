#ifndef STOCKTASK_H
#define STOCKTASK_H
#include <QList>
#include <QThreadPool>
#include <QDebug>
#include "stock/Stock.h"


class StockTask : public QRunnable
{
public:
    Stock*  mStock;
    int   mTaskFlag;
    bool mLast;

    StockTask(Stock* s,int flag):mStock(s),mTaskFlag(flag),mLast(false)
    {
        setAutoDelete(false);//不设置，线程会自动释放掉该runable
    }

    virtual ~StockTask(){}
    virtual void onFinished(){};
};

//历史数据拉取任务
class PullHistoryTask : public StockTask
{
protected:
    QList<KData*> cach;
    int mBeginTime;
    int mEndTime;
public:
    PullHistoryTask(Stock* s, int flag, int beginTime, int endTime):StockTask(s,flag),mBeginTime(beginTime),mEndTime(endTime){}
    void run();
    void onFinished();
};

//股票列表信息拉取任务
class PullStocksTask : public StockTask
{
protected:
    QList<Stock> cach;
public:
    PullStocksTask(int flag):StockTask(nullptr,flag){}
    void run();
    void onFinished();
};

//数据库保存任务
class SaveDBTask : public StockTask
{
protected:
    QList<Stock> cach;
public:
    SaveDBTask(int flag):StockTask(nullptr,flag){}
    void run();
    void onFinished(){};
};

//股票分析任务
class  AnalyseTask : public StockTask
{
protected:
    AnalyseInfo data;
public:
    AnalyseTask(Stock* s,int flag)
        :StockTask(s,flag)
    {}
    void run();
    void onFinished();
};

//校验任务
class  CheckTask : public StockTask
{
public:
    enum Error
    {
        LoseDate=0x0001,//日k数据丢失
    };
    int err;
public:
    CheckTask(Stock* s,int flag)
        :StockTask(s,flag)
    {}
    void run();
    void onFinished();
};

#endif // STOCKTASK_H
