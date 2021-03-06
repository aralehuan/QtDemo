﻿#ifndef STOCKTASK_H
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

//拉取单只股票历史数据
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

//拉取所有股票今日数据(可以快速同步今日数据)
class PullTodayTask : public StockTask
{
protected:
    int date;
    QMap<QString,KData*> cach;
public:
    PullTodayTask(int flag,int day):StockTask(nullptr,flag),date(day){}
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

//分笔数据拉取任务
class PullTDataTask : public StockTask
{
protected:
    KData* kData;
    QList<TData>* cach;
public:
    PullTDataTask(Stock* s,KData *k, int flag):StockTask(s,flag),kData(k){}
    void run();
    void onFinished();
};

//数据库保存任务
class SaveDBTask : public StockTask
{
protected:
    bool commitOK;
public:
    SaveDBTask(int flag):StockTask(nullptr,flag),commitOK(false){}
    void run();
    void onFinished();
};

//股票分析任务
class  AnalyseTask : public StockTask
{
protected:
    int date;//分析某天+以前的数据，这样可以使用第2个交易日的数据做验证
    AnalyseInfo data;
public:
    AnalyseTask(Stock* s,int day, int flag):StockTask(s,flag),date(day)
    {
        mStock->getValidHistory();//防止异步加载，先同步加载
    }
    void run();
    void onFinished();
};

//校验任务
class  CheckTask : public StockTask
{
protected:
    CheckInfo data;
public:
    CheckTask(Stock* s,int flag) :StockTask(s,flag)
    {
        mStock->getHistory();//防止异步加载，先同步加载
    }
    void run();
    void onFinished();
};

#endif // STOCKTASK_H
