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
StockMgr::StockMgr()
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
        connect(this, SIGNAL(sendStockChanged(Stock*)), this, SLOT(onStockChanged(Stock*)), Qt::ConnectionType::QueuedConnection);
    }
    while (false);
    return nullptr;
}

void StockMgr::deinit()
{
    mPool.clear();
    mPool.waitForDone();
    mDB.close();
    Py_Finalize();
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

const QList<Stock*>& StockMgr::getStocks()
{
    if(!mStocks.isEmpty())return mStocks;
    QString err = loadStocks();
    if(!err.isEmpty())
    {
        qDebug()<<err;
    }
    return  mStocks;
}

QString StockMgr::loadStocks()
{
    if(!exist("Stocks"))
    {
        QString err = createTable();
        if(!err.isEmpty())return err;
        err = reqStocks();
        if(!err.isEmpty())return err;
    }
    else
    {
        QSqlQuery query;
        query.prepare(QString("SELECT * From Stocks"));
        if(!query.exec() ) return QString("load stocks failed");
        while(query.next())
        {
            QSqlRecord r = query.record();
            QString code = r.value("code").toString();
            Stock* sk = getStock(code);
            sk->name = r.value("name").toString();
            sk->industry = r.value("industry").toString();
            sk->area = r.value("area").toString();
            sk->total = r.value("total").toDouble();
        }
    }
    return  nullptr;
}

QString StockMgr::reqStocks()
{
    PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_tickets");
    PyObject* pyRet  = PyObject_CallObject(pyFunc, nullptr);
    char* json;
    PyArg_Parse(pyRet,"s",&json);

    //json数据解析
    QJsonParseError json_error;
    QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
    if(json_error.error != QJsonParseError::NoError)return QString("json数据错误");
    QJsonObject rootObj = jdoc.object();
    QJsonArray datas = rootObj.value("data").toArray();

    //写数据库
    //开启事务(先提交到缓存，避免频繁的打开关闭文件操作，一次性写入)
    QString err;
    mDB.transaction();
    for(int i=0;i<datas.count();++i)
    {
        QJsonObject data = datas[i].toObject();
        QString code = data.value("code").toVariant().toString();
        Stock* sk = getStock(code);
        sk->name = data.value("name").toVariant().toString();
        sk->industry = data.value("industry").toVariant().toString();
        sk->area = data.value("area").toVariant().toString();
        sk->area = data.value("totals").toVariant().toFloat();
        sk->out = data.value("outstanding").toVariant().toFloat();
        sk->marketTime = data.value("timeToMarket").toVariant().toInt();

        QString sql;
        QString where = QString("code='%1'").arg(sk->code);
        if(exist("Stocks", where))
        {
            QSqlQuery query;
            query.prepare(QString("UPDATE Stocks SET industry=?,area=?,total=?,out=?,marketTime=? WHERE %1").arg(where));
            query.addBindValue(sk->industry);
            query.addBindValue(sk->area);
            query.addBindValue(sk->total);
            query.addBindValue(sk->out);
            query.addBindValue(sk->marketTime);
            if(!query.exec())
            {
                err = QString("股票代号"+code+"更新记录失败!");
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
                    %7  \
                    );").arg(sk->code).arg(sk->name).arg(sk->industry).arg(sk->area).arg(sk->total).arg(sk->out).arg(sk->marketTime);
            QSqlQuery query;
            query.prepare(sql);
            if(!query.exec())
            {
                err = QString("股票代号"+code+"添加记录失败!");
                break;
            }
        }
    }
    //提交事务
    mDB.commit();
    return err;
}


bool StockMgr::exist(QString table, QString where)
{
    if(where==nullptr)
    {
        QSqlQuery query;
        query.exec(QString("SELECT * FROM sqlite_master WHERE name='%1'").arg(table));
        if(query.exec())return query.next();
    }
    else
    {
        QSqlQuery query;
        query.prepare(QString("SELECT * FROM %1 WHERE %2 LIMIT 1").arg(table).arg(where));
        if(query.exec())return query.next();
    }
    throw QString("sql 执行失败");
}

QString StockMgr::createTable()
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
                timeToMarket INT \
                );";
        QSqlQuery query;
        query.prepare(sql);
        if(!query.exec())return QString("create table failed");
    }while (0);
    return nullptr;
}

const QList<KData*>& Stock::getHistory()
{
    if(dirty)
    {//合并异步请求数据并写数据库
        QString tbName = "X"+code;
        //开启事务(先提交到缓存，避免频繁的打开关闭文件操作，一次性写入)
        QSqlDatabase& db = StockMgr::single()->getDB();
        db.transaction();
        lock.lock();
        for(int i=0;i<cach.size();++i)
        {//最新数据在最前面
            KData* k = cach[i];
            history.insert(0,k);
            if(k->date>maxDate)maxDate=k->date;
            if(k->date<minDate)minDate=k->date;

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
            QSqlQuery query;
            query.prepare(sql);
            if(!query.exec())
            {
                qDebug()<<"股票代号"+code+"添加记录失败!";
                break;
            }
        }
        cach.clear();
        dirty=false;
        lock.unlock();
        if(!db.commit())db.rollback();
    }

    if(maxDate==0)
     {//未加载过
         QString tbName = "X"+code;
         if(!StockMgr::exist(tbName))
         {
             createTable();
         }

         //从数据库取数据
         QSqlQuery query;
         query.prepare(QString("SELECT * From X%1").arg(code));
         if(!query.exec()) throw QString("load stocks failed");
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
    }

    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    if(maxDate<date)
    {//数据不是最新，网络请求
        this->setAutoDelete(false);//不设置，线程会自动释放掉该runable
         StockMgr::single()->startTask(this);
    }
    return history;
}

void Stock::run()
{
    qDebug()<<"req histroy code="+code;
    QDate now = QDate::currentDate();
    QString date = now.toString("yyyy-MM-dd");
    //如果没有数据取今年初到今天的数据，否则取最后一天到今天的数据
    QString begin=history.isEmpty()?QString("%1-01-01").arg(now.year()):history[0]->strDate();

    //拉取最新数据
    PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_ticket");
    PyObject *pArgs = PyTuple_New(3);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", code.toLatin1().data()));
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", begin.toLatin1().data()));
    PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", date.toLatin1().data()));
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
    lock.lock();
    for(int i=datas.count()-1;i>=0;--i)
    {
        QJsonObject data = datas[i].toObject();
        int date = data.value("date").toString().remove('-').toInt();
        if(date>=minDate && date<=maxDate)continue;//已存在

        KData* k = new KData();
        k->date = date;
        k->open = data.value("open").toVariant().toFloat();
        k->high = data.value("high").toVariant().toFloat();
        k->low = data.value("low").toVariant().toFloat();
        k->close = data.value("close").toVariant().toFloat();
        k->volume = data.value("volume").toVariant().toDouble();
        k->ma5 = data.value("ma5").toVariant().toDouble();
        cach.push_back(k);
    }
    dirty=true;
    lock.unlock();
    StockMgr::single()->notifyStockChanged(this);
}

void Stock::createTable()
{
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
        QSqlQuery query;
        query.prepare(sql);
        if(!query.exec())throw  QString("create table failed");
    }while (0);
}
