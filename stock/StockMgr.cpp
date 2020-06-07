#pragma execution_character_set("utf-8")
#include <Python.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include <QDate>
#include "stock/StockMgr.h"

PyObject* gPythonMod=nullptr;
StockMgr* StockMgr::mThis = nullptr;
thread_local ThreadDB threadDB;
StockMgr::StockMgr()
    :mTaskCount(0)
    ,mMinDate(0)
    ,mMaxDate(0)
    ,mSaving(false)
{
    //线程通信信号槽
    connect(this, SIGNAL(sendTaskFinished(QSharedPointer<StockTask>)), this, SLOT(onTaskFinished(QSharedPointer<StockTask>)), Qt::ConnectionType::QueuedConnection);
}

QString StockMgr::init()
{
    do
    {
        //python初始化
        Py_Initialize();
        if(!Py_IsInitialized())return QString("Python init failed");
        gPythonMod = PyImport_ImportModule("tusharedemo");
        if(gPythonMod==nullptr)return QString("tusharedemo.py script error");
        //线城池初始化
        mPool.setMaxThreadCount(1);
        //同步股票列表数据
        loadStocks(threadDB.getDB());
        startTask(new PullStocksTask(TaskFlag::InitList));
        //同步参考股票(其他股票以该股票日期做参考，避免每次无效请求)
        QDate now = QDate::currentDate();
        int cur = now.year()*10000+now.month()*100+now.day();
        Stock* sk = getRefStock();
        QDate min2(sk->minDate/10000, sk->minDate%10000/100, sk->minDate%100);
        QDate min1 = min2.addMonths(-1);
        int minBegin = min1.year()*10000+min1.month()*100+min1.day();
        if(minBegin<OLDEST_YEAR_DAY)minBegin=OLDEST_YEAR_DAY;
        startTask(new PullHistoryTask(sk, TaskFlag::InitMinDate, minBegin, min2.year()*10000+min2.month()*100+min2.day()));
        QDate max2(cur/10000, cur%10000/100, cur%100);
        QDate max1 = max2.addMonths(-1);
        startTask(new PullHistoryTask(sk, TaskFlag::InitMaxDate, max1.year()*10000+max1.month()*100+max1.day(), max2.year()*10000+max2.month()*100+max2.day()));
    }
    while (false);
    return nullptr;
}

void StockMgr::deinit()
{
    threadWake();
    if(mThread!=nullptr)mThread->wait();
    mPool.clear();
    mPool.waitForDone();
    Py_Finalize();
}

Result StockMgr::syncData(Stock* stock)
{
    if(isBusy())return Result::Busy;
    Stock* ref = getRefStock();
    if(ref->minDate<=0||ref->maxDate<=0)return Result::DateError;

    QDate now = QDate::currentDate();
    int cur = now.year()*10000+now.month()*100+now.day();
    int min = stock->minDate;
    int max= stock->maxDate;
     if(max<ref->maxDate && max<cur)
    {//最新数据没有
       if(max<=0)max=now.year()*10000+101;//仅取今年数据
       qDebug()<<"sync "<<stock->code<<":"<<max<<","<<cur;
       startTask(new PullHistoryTask(stock,SyncData,max,cur));
    }

    if(max>0&&min>ref->minDate && min>stock->marketTime && min>OLDEST_YEAR_DAY)
    {//如果有更早的数据，就再取前3年的数据
        int older = min-30000<OLDEST_YEAR_DAY?OLDEST_YEAR_DAY:min-30000;
        qDebug()<<"sync "<<stock->code<<":"<<older<<","<<min;
        startTask(new PullHistoryTask(stock,SyncData,older,min));
    }
    return Result::Ok;
}

Result StockMgr::syncToday()
{
    QDate now = QDate::currentDate();
    QTime time = QTime::currentTime();
    int curDate = now.year()*10000+now.month()*100+now.day();
    if(time.hour()<10||time.hour()>22)return Result::NotInTime;
    if(isBusy())return Result::Busy;
    startTask(new PullTodayTask(SyncData,curDate));
    return Result::Ok;
}

Result StockMgr::saveData()
{
    if(mSaving)return Result::Saving;
    mSaving=true;
    const QList<Stock*>& stocks = StockMgr::single()->getStocks();
    for(int i=0;i<stocks.length();++i)stocks[i]->prepareSave();
    startTask(new SaveDBTask(SaveDB));
    return Result::Ok;
}

Result StockMgr::checkData(Stock* stock)
{
    if(isBusy())return Result::Busy;
    startTask(new CheckTask(stock,TaskFlag::Check));
    return Result::Ok;
}

Result StockMgr::analyseData(Stock* stock)
{
    if(isBusy())return Result::Busy;
    startTask(new AnalyseTask(stock,TaskFlag::Analyse));
    return Result::Ok;
}

Result StockMgr::removeData(int startDate)
{
    if(isBusy())return Result::Busy;
    Stock* ref = StockMgr::single()->getRefStock();
    QSqlDatabase& db = threadDB.getDB();
    db.transaction();
    const QList<Stock*>& ss = StockMgr::single()->getStocks();
    for(int i=0;i<ss.count();++i)
    {
        Stock* s = ss[i];
        if(s==ref)continue;
        s->removeHistory(startDate);
    }
    db.commit();
    return Result::Ok;
}

Stock* StockMgr::getStock(QString code,bool create)
{
    auto i = mStockMap.find(code);
    if(i!=mStockMap.end())return i.value();
    if(!create)return nullptr;
    Stock* s = new Stock(code);
    mStockMap[code]=s;
    mStocks.push_back(s);
    return s;
}

bool StockMgr::exist(QSqlDatabase& db, QString table, QString where)
{
    if(where==nullptr)
    {
        QSqlQuery query(db);
        query.exec(QString("SELECT * FROM sqlite_master WHERE name='%1' limit 1").arg(table));
        if(query.exec())return query.next();
    }
    else
    {
        QSqlQuery query(db);
        query.prepare(QString("SELECT * FROM %1 WHERE %2 LIMIT 1").arg(table).arg(where));
        if(query.exec())return query.next();
    }
    throw QString("sql 执行失败");
}

QString StockMgr::createTable(QSqlDatabase& db)
{
    do
    {
        QString sql = "CREATE TABLE Stocks \
                ( \
                code varchar(10) PRIMARY KEY, \
                name varchar(32) NOT NULL, \
                industry varchar(32), \
                area varchar(32), \
                totals decimal(5,2), \
                outstanding decimal(5,2), \
                timeToMarket INT, \
                minDate INT, \
                maxDate INT, \
                blacklist BOOL default 0 \
                );";
        QSqlQuery query(db);
        query.prepare(sql);
        if(!query.exec())return QString("create table failed");
    }while (0);
    return nullptr;
}

void StockMgr::loadStocks(QSqlDatabase& db)
{
    if(!exist(db, "Stocks"))createTable(db);
    QSqlQuery query(db);
    query.prepare(QString("SELECT * From Stocks order by code asc"));
    if(!query.exec() )throw QString("load stocks failed");
    while(query.next())
    {
        QSqlRecord r = query.record();
        QString code = r.value("code").toString();
        Stock* sk = getStock(code);
        sk->name = r.value("name").toString();
        sk->industry = r.value("industry").toString();
        sk->area = r.value("area").toString();
        sk->total = r.value("totals").toDouble();
        sk->out = r.value("outstanding").toDouble();
        sk->marketTime = r.value("timeToMarket").toInt();
        sk->minDate = r.value("minDate").toInt();
        sk->maxDate = r.value("maxDate").toInt();
        sk->blacklist = r.value("blacklist").toBool();
        sk->created=sk->maxDate!=0;
    }
}

void StockMgr::onTaskFinished(QSharedPointer<StockTask> sharetask)
{
    --mTaskCount;
    StockTask* task = sharetask.get();
    task->onFinished();
    emit sendMessage(TaskCount,"");
    if(task->mTaskFlag == TaskFlag::SaveDB)
    {
        mSaving=false;
    }
}

QString StockMgr::Result2Msg(Result r)
{
    switch(r)
    {
    case Result::Ok:
        return "";
    case Result::Busy:
        return "后台任务忙";
    case Result::Saving:
        return "正在保存数据";
    case Result::DateError:
        return "参考日期错误";
    case Result::NotInTime:
        return "工作时间段为早10点到晚10点之间";
    }
    return "未知错误";
}
