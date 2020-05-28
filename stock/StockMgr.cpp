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
        startTask(new PullStocksTask(TaskFlag::InitSync));
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

void StockMgr::syncData(Stock* stock)
{
    if(isBusy())
    {
        emit sendMessage(MsgType::MessageBox, QString("[同步]正在后台处理数据,稍好再试"));
        return;
    }

    if(mMinDate==0||mMaxDate==0)
    {
        getValideDate(mMinDate, mMaxDate);
    }

    QDate now = QDate::currentDate();
    int cur = now.year()*10000+now.month()*100+now.day();
    if(stock!=nullptr)
    {
        int min = stock->minDate;
        int max= stock->maxDate;
         if(mMaxDate>0 && max<mMaxDate && max<cur)
        {//最新数据没有
           if(max<=0)max=now.year()*10000+101;//仅取今年数据
           qDebug()<<"sync "<<stock->code<<":"<<max<<","<<cur;
           startTask(new PullHistoryTask(stock,SyncData,max,cur));
        }

        if(mMinDate>0 && min>mMinDate && min>stock->marketTime && min>OLDEST_YEAR_DAY)
        {//如果有更早的数据，就再取前3年的数据
            int older = min-30000<OLDEST_YEAR_DAY?OLDEST_YEAR_DAY:min-30000;
            qDebug()<<"sync "<<stock->code<<":"<<older<<","<<min;
            startTask(new PullHistoryTask(stock,SyncData,older,min));
        }
        return;
    }

    for(int i=0;i<mStocks.length();++i)
    {
        Stock* stock = mStocks[i];
        if(stock->blacklist)continue;
        int min = stock->minDate;
        int max= stock->maxDate;
        if(mMaxDate>0 && max<mMaxDate && max<cur)
        {//最新数据没有
           if(max<=0)max=now.year()*10000+101;//仅取今年数据
           qDebug()<<"sync "<<stock->code<<":"<<max<<","<<cur;
           startTask(new PullHistoryTask(stock,SyncData,max,cur));
        }

        if(mMinDate>0 && min>mMinDate && min>stock->marketTime && min>OLDEST_YEAR_DAY)
        {//如果有更早的数据，就再取前3年的数据
            int older = min-30000<OLDEST_YEAR_DAY?OLDEST_YEAR_DAY:min-30000;
            qDebug()<<"sync "<<stock->code<<":"<<older<<","<<min;
            startTask(new PullHistoryTask(stock,SyncData,older,min));
        }
    }
}

void StockMgr::saveData()
{
    if(isBusy())
    {
        emit sendMessage(MsgType::MessageBox, QString("[保存]正在后台处理数据,稍好再试"));
        return;
    }
    startTask(new SaveDBTask(SaveDB));
}

void StockMgr::checkData(Stock* stock)
{
    if(isBusy())
    {
        emit sendMessage(MsgType::MessageBox, QString("[校验]正在后台处理数据,稍好再试"));
        return;
    }

    if(stock!=nullptr)
    {
        startTask(new CheckTask(stock,TaskFlag::Check));
        return;
    }

    for(int i=0;i<mStocks.length();++i)
    {
        Stock* sk = mStocks[i];
        if(sk->blacklist)continue;
        startTask(new CheckTask(sk,TaskFlag::Check));
    }
}

void StockMgr::analyseData(Stock* stock)
{
    if(isBusy())
    {
        emit sendMessage(MsgType::MessageBox, QString("[分析]正在后台处理数据,稍好再试"));
        return;
    }

    if(stock!=nullptr)
    {
        startTask(new AnalyseTask(stock,TaskFlag::Analyse));
        return;
    }

    for(int i=0;i<mStocks.length();++i)
    {
        Stock* sk = mStocks[i];
        if(sk->blacklist)continue;
        startTask(new AnalyseTask(sk,TaskFlag::Analyse));
    }
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

void StockMgr::getValideDate(int& minDate, int& maxDate)
{
    minDate = maxDate = 0;
    QDate now = QDate::currentDate();
    int cur = now.year()*10000+now.month()*100+now.day();
    Stock* sk = getStock("000001");
    QDate date(sk->minDate/10000, sk->minDate%10000/100, sk->minDate%100);
    QDate date2 = date.addMonths(-1);
    QString begin = StockMgr::int2StrTime(date2.year()*10000+date2.month()*100+date2.day());
    QString end   = StockMgr::int2StrTime(date.year()*10000+date.month()*100+date.day());
    PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_ticket");
    PyObject *pArgs = PyTuple_New(4);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", "000001"));
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", begin.toLatin1().data()));
    PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", end.toLatin1().data()));
    PyTuple_SetItem(pArgs, 3, Py_BuildValue("i", sk->market()));
    PyObject* pyRet  = PyObject_CallObject(pyFunc, pArgs);
    do
    {
        char* json;
        PyArg_Parse(pyRet,"s",&json);
        if(json==nullptr)break;
        QJsonParseError json_error;
        QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
        if(json_error.error != QJsonParseError::NoError)break;
        QJsonObject rootObj = jdoc.object();
        QJsonArray datas = rootObj.value("data").toArray();
        if(datas.count()<1)break;
        minDate = StockMgr::str2IntTime(datas[datas.count()-1].toObject().value("date").toString());
    }while(false);


    QDate date3(cur/10000, cur%10000/100, cur%100);
    QDate date4 = date3.addMonths(-1);
    begin = StockMgr::int2StrTime(date4.year()*10000+date4.month()*100+date4.day());
    end   = StockMgr::int2StrTime(date3.year()*10000+date3.month()*100+date3.day());
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", begin.toLatin1().data()));
    PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", end.toLatin1().data()));
    pyRet  = PyObject_CallObject(pyFunc, pArgs);
    do
    {
        char* json;
        PyArg_Parse(pyRet,"s",&json);
        if(json==nullptr)break;
        QJsonParseError json_error;
        QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
        if(json_error.error != QJsonParseError::NoError)break;
        QJsonObject rootObj = jdoc.object();
        QJsonArray datas = rootObj.value("data").toArray();
        if(datas.count()<1)break;
        maxDate = StockMgr::str2IntTime(datas[0].toObject().value("date").toString());
    }while(false);
    qDebug()<<"valide date range:"<<minDate<<"->"<<maxDate;
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
    if(task->mTaskFlag==SyncData && mTaskCount<1)
    {
        saveData();
    }
}
