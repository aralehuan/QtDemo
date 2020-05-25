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
StockMgr::StockMgr():mTaskCount(0),mSaving(false)
{
}

QString StockMgr::init()
{
    do
    {
        //数据库初始化
        mDB = QSqlDatabase::addDatabase("QSQLITE");
        mDB.setDatabaseName("StockDB.db");
        if(!mDB.open())return QString("database connect failed");
        //python初始化
        Py_Initialize();
        if(!Py_IsInitialized())return QString("Python init failed");
        gPythonMod = PyImport_ImportModule("tusharedemo");
        if(gPythonMod==nullptr)return QString("tusharedemo.py script error");
        //线城池初始化
        mPool.setMaxThreadCount(1);
        //线程通信信号槽
        connect(this, SIGNAL(sendTaskFinished(StockTask*)), this, SLOT(onTaskFinished(StockTask*)), Qt::ConnectionType::QueuedConnection);
        //同步股票列表数据
        loadStocks(mDB);
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
    mDB.close();
    Py_Finalize();
}

void StockMgr::syncData()
{
    if(mSaving)return;
    int minDate, maxDate;
    getValideDate(minDate, maxDate);
    QDate now = QDate::currentDate();
    int cur = now.year()*10000+now.month()*100+now.day();
    for(int i=0;i<mStocks.length();++i)
    {
        Stock* sk = mStocks[i];
        int min = sk->minDate;
        int max= sk->maxDate;
        if(maxDate>0 && max<maxDate && max<cur)
        {//最新数据没有
           if(max<=0)max=now.year()*10000+101;//仅取今年数据
           qDebug()<<"sync "<<sk->code<<":"<<max<<","<<cur;
           startTask(new PullHistoryTask(sk,SyncData,max,cur));
        }

        if(minDate>0 && min>minDate && min>sk->marketTime && min>OLDEST_YEAR_DAY)
        {//如果有更早的数据，就再取前3年的数据
            int older = min-30000<OLDEST_YEAR_DAY?OLDEST_YEAR_DAY:min-30000;
            qDebug()<<"sync "<<sk->code<<":"<<older<<","<<min;
            startTask(new PullHistoryTask(sk,SyncData,older,min));
        }
    }
}

void StockMgr::saveData()
{
    qDebug()<<"save begin";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "savedb");//不能同名，同个连接不能跨线程使用
    db.setDatabaseName("StockDB.db");
    if(!db.open())
    {
        emit sendMessage(MsgType::Error, QString("创建数据库连接savedb失败"));
        return;
    }
    //开启事务(先提交到缓存，避免频繁的打开关闭文件操作，一次性写入)
    db.transaction();
    QSqlQuery query(db);

    emit sendMessage(MsgType::Info, QString("保存股票列表信息"));
    for(int i=0;i<mStocks.length();++i)
    {
        Stock* sk = mStocks[i];
        if(!sk->dirty)continue;
        QString sql;
        QString where = QString("code='%1'").arg(sk->code);
        if(exist(db, "Stocks", where))
        {
            query.prepare(QString("UPDATE Stocks SET industry=?,area=?,totals=?,outstanding=?,timeToMarket=?,minDate=?,maxDate=? WHERE %1").arg(where));
            query.addBindValue(sk->industry);
            query.addBindValue(sk->area);
            query.addBindValue(sk->total);
            query.addBindValue(sk->out);
            query.addBindValue(sk->marketTime);
            query.addBindValue(sk->minDate);
            query.addBindValue(sk->maxDate);
            if(!query.exec())
            {
                emit sendMessage(MsgType::Error, QString("股票代号"+sk->code+"更新记录失败!"));
                break;
            }
        }
        else
        {
            sql = QString("INSERT INTO Stocks VALUES \
                    (  \
                    '%1', \
                    '%2', \
                    '%3', \
                    '%4', \
                    %5, \
                    %6, \
                    %7, \
                    %8, \
                    %9, \
                    );").arg(sk->code).arg(sk->name).arg(sk->industry).arg(sk->area).arg(sk->total).arg(sk->out).arg(sk->marketTime).arg(sk->minDate).arg(sk->maxDate);
            query.prepare(sql);
            if(!query.exec())
            {
                emit sendMessage(MsgType::Error, QString("股票代号"+sk->code+"添加记录失败!"));
                break;
            }
        }
    }

    emit sendMessage(MsgType::Info, QString("保存股票历史数据"));
    for(int i=0;i<mStocks.length();++i)
    {
        Stock* sk = mStocks[i];
        sk->createTable(db);
        const QList<KData*>& ls = sk->history;
        QString tbName = "X"+sk->code;
        for(int m=0;m<ls.size();++m)
        {//最新数据在最前面
            KData* k = ls[m];
            if(!k->dirty)continue;
            QString sql = QString("INSERT OR IGNORE INTO %1 (date,high,open,close,low,volume,ma5) VALUES \
                   (  \
                   '%2', \
                   %3, \
                   %4, \
                   %5, \
                   %6, \
                   %7, \
                   %8  \
                   );").arg(tbName).arg(k->date).arg(k->high).arg(k->open).arg(k->close).arg(k->low).arg(k->volume).arg(k->ma5);
            query.prepare(sql);
            if(!query.exec())
            {
                emit sendMessage(MsgType::Error, QString("股票代号"+sk->code+"添加历史记录失败 date="+k->date));
                break;
            }
        }
    }
    //提交事务
    if(!db.commit())
    {
        qDebug()<<"save db rollback";
        db.rollback();
        emit sendMessage(MsgType::Error, QString("数据保存失败，事务回滚"));
    }
    else
    {
        for(int i=0;i<mStocks.length();++i)
        {
                Stock* sk = mStocks[i];
                sk->dirty=false;
                const QList<KData*>& ls = sk->history;
                for(int m=0;m<ls.size();++m)ls[m]->dirty=false;
        }
    }
    db.close();
    qDebug()<<"save end";
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
    PyObject *pArgs = PyTuple_New(3);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", "000001"));
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", begin.toLatin1().data()));
    PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", end.toLatin1().data()));
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
                created BOOL \
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
        sk->marketTime = r.value("timeToMarket").toInt();
        sk->minDate = r.value("minDate").toInt();
        sk->maxDate = r.value("maxDate").toInt();
        sk->created=sk->maxDate!=0;
    }
}

void StockMgr::onTaskFinished(StockTask* task)
{
    --mTaskCount;
    task->onFinished();
    if(task->mTaskFlag==SaveDB)
    {
        mSaving=false;
    }
    else if(task->mTaskFlag==SyncData && mTaskCount<1)
    {
        mSaving=true;
        startTask(new SaveDBTask(SaveDB));
    }
    delete  task;
    emit sendMessage(TaskCount,"");
}


const QList<KData*>& Stock::getHistory()
{
     if(loaded)return history;
     //从数据库取数据
     QSqlQuery query;
     query.prepare(QString("SELECT * From X%1 order by date desc").arg(code));
     if(!query.exec())return history;//可能表没建立
     while(query.next())
     {
         QSqlRecord r = query.record();
         KData* k = new KData();
         k->date = r.value("date").toInt();
         k->high = r.value("high").toFloat();
         k->low = r.value("low").toFloat();
         k->open = r.value("open").toFloat();
         k->close = r.value("close").toFloat();
         k->volume = r.value("volume").toDouble();
         k->ma5 =  r.value("ma5").toFloat();
         history.append(k);
         if(k->date>maxDate)maxDate=k->date;
         if(k->date<minDate)minDate=k->date;
     }
     std::sort(history.begin(),history.end(),[](const KData* a,const KData* b){return a->date>b->date;});
     loaded=true;
    return history;
}

void Stock::mergeHistory(const QList<KData*>& ls)
{
    for(int i=0;i<ls.size();++i)
    {//最新数据在最前面
        KData* k = ls[i];
        if(k->date>=minDate && k->date<=maxDate)continue;//已存在
        history.insert(0,k);
        if(k->date>maxDate)maxDate=k->date;
        if(k->date<minDate)minDate=k->date;
        dirty = true;
        k->dirty=true;
    }
}

void Stock::createTable(QSqlDatabase& db)
{
    if(created)return;
    do
    {
        QString sql = QString("CREATE TABLE X%1 \
                ( \
                date INT PRIMARY KEY, \
                high decimal(5,2), \
                open decimal(5,2), \
                close decimal(5,2), \
                low decimal(5,2), \
                volume decimal(8,2), \
                ma5 decimal(5,2) \
                );").arg(code);
        QSqlQuery query(db);
        query.prepare(sql);
        if(!query.exec())
        {
            if(!StockMgr::exist(db, "X"+code))throw  QString("create table failed");
        }
    }while (0);
    created=true;
}


void PullHistoryTask::run()
{
    QString begin = StockMgr::int2StrTime(mBeginTime);
    QString end   = StockMgr::int2StrTime(mEndTime);
    qDebug()<<"req histroy code="+mStock->code<<",begin="<<begin<<",end="<<end;
    //拉取最新数据
    PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_ticket");
    PyObject *pArgs = PyTuple_New(3);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", mStock->code.toLatin1().data()));
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", begin.toLatin1().data()));
    PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", end.toLatin1().data()));
    PyObject* pyRet  = PyObject_CallObject(pyFunc, pArgs);
    char* json;
    PyArg_Parse(pyRet,"s",&json);
    if(json==nullptr)return;

    //json数据解析
    QJsonParseError json_error;
    QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
    if(json_error.error != QJsonParseError::NoError)return;
    QJsonObject rootObj = jdoc.object();
    QJsonArray datas = rootObj.value("data").toArray();

    //更新数据
    for(int i=datas.count()-1;i>=0;--i)
    {
        QJsonObject data = datas[i].toObject();
        KData* k = new KData();
        k->date = StockMgr::str2IntTime(data.value("date").toString());
        k->open = data.value("open").toVariant().toFloat();
        k->high = data.value("high").toVariant().toFloat();
        k->low = data.value("low").toVariant().toFloat();
        k->close = data.value("close").toVariant().toFloat();
        k->volume = data.value("volume").toVariant().toDouble();
        k->ma5 = data.value("ma5").toVariant().toDouble();
        cach.push_back(k);
    }
    StockMgr::single()->notifyTaskFinished(this);
}

void PullHistoryTask::onFinished()
{
    mStock->mergeHistory(cach);
}

void PullStocksTask::run()
{
    PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_tickets");
    PyObject* pyRet  = PyObject_CallObject(pyFunc, nullptr);
    char* json;
    PyArg_Parse(pyRet,"s",&json);
    QJsonParseError json_error;
    QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
    if(json_error.error == QJsonParseError::NoError)
    {
        QJsonObject rootObj = jdoc.object();
        QJsonArray datas = rootObj.value("data").toArray();
        for(int i=0;i<datas.count();++i)
        {
            QJsonObject data = datas[i].toObject();
            Stock sk(data.value("code").toVariant().toString());
            sk.name = data.value("name").toVariant().toString();
            sk.industry = data.value("industry").toVariant().toString();
            sk.area = data.value("area").toVariant().toString();
            sk.total = data.value("totals").toVariant().toDouble();
            sk.out = data.value("outstanding").toVariant().toDouble();
            sk.marketTime = data.value("timeToMarket").toVariant().toInt();
            cach.push_back(sk);
        }
    }
    StockMgr::single()->notifyTaskFinished(this);
}

void PullStocksTask::onFinished()
{
    for(int i=0;i<cach.size();++i)
    {
        Stock& ts = cach[i];
        *StockMgr::single()->getStock(ts.code) = ts;
    }
}

void SaveDBTask::run()
{
    StockMgr::single()->saveData();
}
