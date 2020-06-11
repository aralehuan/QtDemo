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

void PullTodayTask::run()
{
    //获取股票数
    PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_today_stock_count");
    PyObject* pyRet  = PyObject_CallObject(pyFunc, nullptr);
    int count=0;
    PyArg_Parse(pyRet,"i",&count);
    //分页获取股票数据
    int pageSize = 500;
    int pages=(count+pageSize-1)/pageSize;
    pyFunc = PyObject_GetAttrString(gPythonMod,"get_today_all");
    for(int i=0;i<pages;++i)
    {
        PyObject *pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, Py_BuildValue("i", i+1));
        PyTuple_SetItem(pArgs, 1, Py_BuildValue("i", pageSize));
        PyObject* pyRet  = PyObject_CallObject(pyFunc, pArgs);
        char* json;
        PyArg_Parse(pyRet,"s",&json);
        QJsonParseError json_error;
        QJsonDocument jdoc = QJsonDocument::fromJson(json, &json_error);
        if(json_error.error != QJsonParseError::NoError)break;
        QJsonObject rootObj = jdoc.object();
        QJsonArray datas = rootObj.value("data").toArray();
        for(int i=0;i<datas.count();++i)
        {
            QJsonObject data = datas[i].toObject();
            QString code = data.value("code").toVariant().toString();
            KData* k = new KData();
            k->date  = date;
            k->open = data.value("open").toVariant().toFloat();
            k->high  = data.value("high").toVariant().toFloat();
            k->low    = data.value("low").toVariant().toFloat();
            k->close = data.value("close").toVariant().toFloat();
            k->volume = data.value("volume").toVariant().toDouble()*100;//东方财富以手为单位1手100股
            k->amount = data.value("amount").toVariant().toDouble();
            cach[code]=k;
        }

        QThread::msleep(1000);//一秒爬一次
    }
    StockMgr::single()->notifyTaskFinished(this);
}

void PullTodayTask::onFinished()
{
    QMap<QString, KData*>::iterator it = cach.begin();
     while (it != cach.end())
     {
         KData* k = it.value();
         Stock* s = StockMgr::single()->getStock(it.key(),false);
         if(s!=nullptr)
         {
             QList<KData*> ls;
             ls.push_back(k);
             s->mergeHistory(ls);
         }
         it++;
     }
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

void PullTDataTask::run()
{
    do
    {
        QString date = StockMgr::int2StrTime(kData->date);
        qDebug()<<"req TData code="+mStock->code<<",date="<<date;
        //拉取最新数据
        PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_ticket_tt");
        PyObject *pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", mStock->code.toLatin1().data()));
        PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", date.toLatin1().data()));
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
        cach = new QList<TData>();
        for(int i=0;i<datas.count();++i)
        {//头部时间最新
            QJsonObject data = datas[i].toObject();
            TData t;
            t.time = data.value("time").toString().remove(':').toInt();
            t.price= data.value("price").toVariant().toFloat();
            t.volume = data.value("volume").toVariant().toDouble();
            t.amount  = data.value("amount").toVariant().toDouble ();
            QString s = data.value("side").toVariant().toString();
            if(s.startsWith( "买"))t.side=1;
            else if(s.startsWith("卖"))t.side=-1;
            cach->push_back(t);
        }
    }while(0);
    StockMgr::single()->notifyTaskFinished(this);
}

void PullTDataTask::onFinished()
{
    kData->timeData = cach;
}

void SaveDBTask::run()
{
    qDebug()<<"save begin";
    StockMgr::single()->notifyTaskStart(this);
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
    commitOK = db.commit();
    if(!commitOK)
    {
        qDebug()<<"save db rollback";
        db.rollback();
        StockMgr::single()->notifyMessage(MsgType::Error, QString("数据保存失败，事务回滚"));
    }
    qDebug()<<"save end";
    StockMgr::single()->notifyTaskFinished(this);
}

void SaveDBTask::onFinished()
{
    if(!commitOK)return;
    const QList<Stock*>& stocks = StockMgr::single()->getStocks();
    for(int i=0;i<stocks.length();++i)stocks[i]->clearDirty();
}

void CheckTask::run()
{
    //参考股
    Stock* ref = StockMgr::single()->getRefStock();
    const QList<KData*>& refHis = ref->getHistory();

    if(mStock->maxDate<ref->maxDate)
    {
        data.state |= StockState::DataNotNew;
    }

    const QList<KData*>& ls = mStock->getHistory();
    int count = ls.count();
    if(count>0)
    {
        data.firstDate = ls[0]->date;
        int m = 0;
        while(m<refHis.count() && refHis[m]->date>ls[0]->date)++m;
        for(int i=0;i<ls.length();++i)
        {
             KData* kd = ls[i];
             if(kd->avg<kd->low-0.01f || kd->avg>kd->high+0.01f)
             {
                 data.state |= StockState::DataError;
                 data.firstErrorDate = kd->date;
                 break;
             }

             if(kd->date!=refHis[m++]->date)
             {
                 data.state |= StockState::DataLose;
                 data.firstLoseDate = kd->date;
                 break;
             }
        }
    }
    StockMgr::single()->notifyTaskFinished(this);
}

void CheckTask::onFinished()
{
    mStock->mCheckInfo = data;
}


void AnalyseTask::run()
{
    //低量连涨
    //板块联动
    //支撑上涨
    //大单小涨
    //分析长牛股特点
    std::chrono::time_point<std::chrono::high_resolution_clock> p0 = std::chrono::high_resolution_clock::now();
    QDate now = QDate::currentDate();
    //=============
    mStock->calculate();
    const QList<KData*>& ls = mStock->getValidHistory();
    int count = ls.count();
    int start=0;
    if(date>0)while(start<count&&ls[start]->date>date)++start;
    if(start>=count)goto exit;

    data.lastKData = ls[start];
    if(start>0)
    {//次日结果
        data.dayResult = ls[start-1]->change;
    }

    if(start>=7)
    {//周内结果
        data.weekResult = (ls[start-7]->close-data.lastKData->close)/data.lastKData->close*100;
    }

    if(start+1<count)
    {//放量指标
        data.volumeRate = ls[start]->volume/ls[start+1]->volume;
    }

    //近日连涨
    float lastPrice = ls[start]->close;
    for(int i=start;i<count;++i)
    {
        KData* kd = ls[i];
        if(kd->change<0)
        {
            data.continueRiseRate = (lastPrice-kd->close)/kd->close*100;
            break;
        }
        ++data.continueRiseDay;
    }

    //年月日涨跌比
    int year   = data.lastKData->date/10000;
    int month= data.lastKData->date%10000/100;
    for(int i=start;i<count;++i)
    {
         KData* kd = ls[i];
        //本年
        if(kd->date/10000 > year)continue;
        if(kd->date/10000 < year)break;
        if(kd->change > 0)
            ++data.yearUp;
        else if(kd->change < 0)
            ++data.yearDown;

        //本月
        int month = kd->date%10000/100;
        if(month != month)continue;
        if(kd->change > 0)
            ++data.monthUp;
        else if(kd->change < 0)
            ++data.monthDown;

        //近7天
        if(i>=start+7)continue;
        if(kd->change > 0)
            ++data.sevenUp;
        else if(kd->change < 0)
            ++data.sevenDown;
    }
    //=============
exit:
    std::chrono::time_point<std::chrono::high_resolution_clock> p1 = std::chrono::high_resolution_clock::now();
    float ms = (float)std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count() / 1000;
    qDebug()<<"analyse "<<mStock->code<<"use "<<ms <<"ms";
    StockMgr::single()->notifyTaskFinished(this);
}

void AnalyseTask::onFinished()
{
    mStock->mAnalyseInfo = data;
}

