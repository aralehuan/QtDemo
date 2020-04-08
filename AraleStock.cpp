#include <Python.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include "AraleStock.h"

AraleStock::AraleStock()
{

}

QString AraleStock::init()
{
    do
    {
        mDB = QSqlDatabase::addDatabase("QSQLITE");
        mDB.setDatabaseName("StockDB.db");
        if(!mDB.open())return QString("database connect failed");
        Py_Initialize();
        if(!Py_IsInitialized())return QString("Python init failed");
        mThread.start();
        this->moveToThread(&mThread);
    }
    while (false);
    return nullptr;
}

void AraleStock::deinit()
{
    mThread.quit();
    mThread.wait();
    mDB.close();
    Py_Finalize();
}

QString AraleStock::loadStocks()
{
    mStocks.clear();
    QSqlQuery query;
    query.prepare(QString("SELECT * From Stocks"));
    if(!query.exec()) return QString("load stocks failed");
    while(query.next())
    {
        QSqlRecord r = query.record();
        Stock* sk = new Stock();
        sk->code = r.value("code").toString();
        sk->name = r.value("name").toString();
        mStocks.append(sk);
        loadKData(sk);
    }
    return  nullptr;
}

QString AraleStock::loadKData(Stock* stock)
{
    stock->history.clear();
    QSqlQuery query;
    query.prepare(QString("SELECT * From X%1").arg(stock->code));
    if(!query.exec()) return QString("load stocks failed");
    while(query.next())
    {
        QSqlRecord r = query.record();
        KData* k = new KData();
        k->date = r.value("date").toString();
        k->high = r.value("high").toFloat();
        k->low = r.value("low").toFloat();
        k->open = r.value("open").toFloat();
        k->close = r.value("close").toFloat();
        k->volume = r.value("volume").toDouble();
        stock->history.insert(0,k);
    }
    return  nullptr;
}

QString AraleStock::reqStocks()
{
    PyObject* pyMod = PyImport_ImportModule("tusharedemo");
    PyObject* pyFunc = PyObject_GetAttrString(pyMod,"get_ticket");
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
        QString name = data.value("name").toVariant().toString();
        QString industry = data.value("industry").toVariant().toString();
        QString area = data.value("area").toVariant().toString();
        float total = data.value("totals").toVariant().toFloat();
        float out = data.value("outstanding").toVariant().toFloat();
        int   marketTime = data.value("timeToMarket").toVariant().toInt();

        QString sql;
        QString where = QString("code='%1'").arg(code);
        if(exist("Stocks", where))
        {
            QSqlQuery query;
            query.prepare(QString("UPDATE Stocks SET industry=?,area=?,total=?,out=?,marketTime=? WHERE %1").arg(where));
            query.addBindValue(industry);
            query.addBindValue(area);
            query.addBindValue(total);
            query.addBindValue(out);
            query.addBindValue(marketTime);
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
                    );").arg(code).arg(name).arg(industry).arg(area).arg(total).arg(out).arg(marketTime);
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

QString AraleStock::reqStock(QString code)
{
    PyObject* pyMod = PyImport_ImportModule("tusharedemo");
    PyObject* pyFunc = PyObject_GetAttrString(pyMod,"get_ticket");
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", code.toLatin1().data()));
    PyObject* pyRet  = PyObject_CallObject(pyFunc, pArgs);
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
    QList<KData*> ls;
    QString err;
    QString tbName = "X"+code;
    if(!exist(tbName))err = createStockTable(tbName);
    mDB.transaction();
    for(int i=0;i<datas.count();++i)
    {
        QJsonObject data = datas[i].toObject();
        KData* k = new KData();
        k->date = data.value("date").toVariant().toString();
        k->open = data.value("open").toVariant().toFloat();
        k->high = data.value("high").toVariant().toFloat();
        k->low = data.value("low").toVariant().toFloat();
        k->close = data.value("close").toVariant().toFloat();
        k->volume = data.value("volume").toVariant().toDouble();
        ls.append(k);
        QString sql = QString("INSERT OR IGNORE INTO %1 (date,high,open,close,low,volume) VALUES \
                (  \
                '%2', \
                %3, \
                %4, \
                %5, \
                %6, \
                %7 \
                );").arg(tbName).arg(k->date).arg(k->high).arg(k->open).arg(k->close).arg(k->low).arg(k->volume);
        QSqlQuery query;
        query.prepare(sql);
        if(!query.exec())
        {
            err = QString("股票代号"+code+"添加记录失败!");
            break;
        }
    }
    if(mDB.commit())
    {
        for(int i=0;i<mStocks.count();++i)
        {
            Stock* s = mStocks[i];
            if(s->code == code)
            {
                s->history.append(ls);
                break;
            }
        }
    }
    else
    {
        mDB.rollback();
    }
    return err;
}

bool AraleStock::exist(QString table, QString where)
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

QString AraleStock::createStockTable()
{
    do
    {
        QString sql = "CREATE TABLE Stocks \
                ( \
                code varchar(10) NOT NULL, \
                name varchar(32) NOT NULL, \
                total decimal(5,2) \
                );";
        QSqlQuery query;
        query.prepare(sql);
        if(!query.exec())return QString("create table failed");
    }while (0);
    return nullptr;
}

QString AraleStock::createStockTable(QString tbName)
{
    do
    {
        QString sql = QString("CREATE TABLE %1 \
                ( \
                date varchar(12) NOT NULL, \
                high decimal(5,2), \
                open decimal(5,2), \
                close decimal(5,2), \
                low decimal(5,2), \
                volume decimal(8,2) \
                );").arg(tbName);
        QSqlQuery query;
        query.prepare(sql);
        if(!query.exec())return QString("create table failed");
    }while (0);
    return nullptr;
}