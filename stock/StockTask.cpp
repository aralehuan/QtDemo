#pragma execution_character_set("utf-8")
#include <Python.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include <QDate>
#include "StockTask.h"
#include "stock/StockMgr.h"

extern PyObject* gPythonMod;
void PullHistoryTask::run()
{
    do
    {
        QThread::msleep(100);//避免过度爬取数据
        QString begin = StockMgr::int2StrTime(mBeginTime);
        QString end   = StockMgr::int2StrTime(mEndTime);
        qDebug()<<"req histroy code="+mStock->code<<",begin="<<begin<<",end="<<end;
        //拉取最新数据
        PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_ticket");
        PyObject *pArgs = PyTuple_New(4);
        PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", mStock->code.toLatin1().data()));
        PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", begin.toLatin1().data()));
        PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", end.toLatin1().data()));
        PyTuple_SetItem(pArgs, 3, Py_BuildValue("i", mStock->market()));
        PyObject* pyRet  = PyObject_CallObject(pyFunc, pArgs);
        char* json;
        PyArg_Parse(pyRet,"s",&json);
        if(json==nullptr)break;

        //json数据解析
        QJsonParseError json_error;
        QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
        if(json_error.error != QJsonParseError::NoError)break;
        QJsonObject rootObj = jdoc.object();
        QJsonArray datas = rootObj.value("data").toArray();
        //更新数据
        for(int i=0;i<datas.count();++i)
        {//头部时间最新
            QJsonObject data = datas[i].toObject();
            KData* k = new KData();
            k->date = StockMgr::str2IntTime(data.value("date").toString());
            k->open = data.value("open").toVariant().toFloat();
            k->high = data.value("high").toVariant().toFloat();
            k->low = data.value("low").toVariant().toFloat();
            k->close = data.value("close").toVariant().toFloat();
            k->volume = data.value("volume").toVariant().toDouble();
            k->amount = data.value("amount").toVariant().toDouble();
            cach.push_back(k);
        }
    }while(0);
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
    qDebug()<<"save begin";
    QSqlDatabase& db = threadDB.getDB();
    //开启事务(先提交到缓存，避免频繁的打开关闭文件操作，一次性写入)
    db.transaction();
    const QList<Stock*>& stocks = StockMgr::single()->getStocks();
    for(int i=0;i<stocks.length();++i)
    {
        Stock* sk = stocks[i];
        if(!sk->save())break;
    }
    //提交事务
    if(db.commit())
    {
        for(int i=0;i<stocks.length();++i)stocks[i]->clearDirty();
    }
    else
    {
        qDebug()<<"save db rollback";
        db.rollback();
        StockMgr::single()->notifyMessage(MsgType::Error, QString("数据保存失败，事务回滚"));
    }
    qDebug()<<"save end";
    StockMgr::single()->notifyTaskFinished(this);
}

void AnalyseTask::run()
{
    //低量连涨
    //板块联动
    //支撑上涨
    //大单小涨
    QDate now = QDate::currentDate();
    const QList<KData*>& ls = mStock->getHistory();
    for(int i=0;i<ls.size();++i)
    {
        KData* kd = ls[i];
        if(kd->date/10000 > now.year())continue;
        if(kd->date/10000 < now.year())break;
        if(kd->close > kd->open)
            ++data.yearUp;
        else if(kd->close < kd->open)
            ++data.yearDown;
        int month = kd->date%10000/100;
        if(month == now.month())
        {
            if(kd->close > kd->open)
                ++data.monthUp;
            else if(kd->close < kd->open)
                ++data.monthDown;
        }
    }
    StockMgr::single()->notifyTaskFinished(this);
}

void AnalyseTask::onFinished()
{
    mStock->mAnalyseInfo = data;
}

void CheckTask::run()
{
    //参考
    const QList<KData*>& ck = StockMgr::single()->getStock("000001")->getHistory();//保证000001数据是最全的
    const QList<KData*>& ls = mStock->getHistory();
    for(int i=0,m=0;i>ck.count()&&m<ls.size();++i)
    {
        if(ck[i]->date>ls[0]->date)continue;
        if(ck[i]->date == ls[m++]->date)continue;
        err|=Error::LoseDate;
        break;
    }
    StockMgr::single()->notifyTaskFinished(this);
}

void CheckTask::onFinished()
{

}
