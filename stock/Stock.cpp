#pragma execution_character_set("utf-8")
#include <Python.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "Stock.h"
#include "StockMgr.h"
#include "StockTask.h"
#include "utility/NanoLog.hpp"
extern PyObject* gPythonMod;
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
                amount decimal(8,2) \
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

const QList<KData*>& Stock::getHistory()
{
     if(loaded)return history;
     //从数据库取数据
     QSqlQuery  query(threadDB.getDB());
     query.prepare(QString("SELECT * From X%1 order by date desc").arg(code));
     if(!query.exec())return history;
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
         k->amount =  r.value("amount").toDouble();
         k->avg = k->amount/k->volume;
         k->turnover = (out==0?0:k->volume/out)*0.000001;
         history.append(k);
         if(k->date>maxDate)maxDate=k->date;
         if(k->date<minDate)minDate=k->date;
     }
     std::sort(history.begin(),history.end(),[](const KData* a,const KData* b){return a->date>b->date;});
     loaded=true;
     validHistory.clear();
    return history;
}

const QList<KData*>& Stock::getValidHistory()
{
    if(!validHistory.isEmpty())return validHistory;
    const QList<KData*> ls =  getHistory();
    for(int i=0;i<ls.size();++i)
    {
        KData* k = ls[i];
        if(k->high==0)continue;//停盘数据
        validHistory.push_back(k);
    }
    return validHistory;
}

const QList<TData>* Stock::getTData(int date)
{
    const QList<KData*> ls =  getValidHistory();
    for(int i=0;i<ls.size();++i)
    {
        KData* k = ls[i];
        if(k->date!=date)continue;
        if(k->timeData!=nullptr)return k->timeData;

        //StockMgr::single()->startTask(new PullTDataTask(this,k,TaskFlag::SyncTData));
        QString sdate = StockMgr::int2StrTime(date);
        qDebug()<<"req TData code="+code<<",date="<<date;
        //拉取最新数据
        PyObject* pyFunc = PyObject_GetAttrString(gPythonMod,"get_ticket_tt");
        PyObject *pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", code.toLatin1().data()));
        PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", sdate.toLatin1().data()));
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
        k->timeData = new QList<TData>();
        for(int i=0;i<datas.count();++i)
        {//头部时间最新
            QJsonObject data = datas[i].toObject();
            TData t;
            t.time = data.value("time").toString().remove(':').toInt();
            t.price= data.value("price").toVariant().toFloat();
            t.volume = data.value("volume").toVariant().toDouble();
            t.amount  = data.value("amount").toVariant().toDouble ();
            QString s = data.value("type").toVariant().toString();
            if(s.startsWith("买"))t.side=1;
            else if(s.startsWith("卖"))t.side=-1;
            k->timeData->push_back(t);
        }
        LOG_INFO <<"pull timedata date="<<date<<" count="<<k->timeData->size();
        return k->timeData;
    }
    return nullptr;
}

bool Stock::save()
{
    if(waitSave==nullptr)return true;
    StockMgr::single()->sendMessage(MsgType::Info, QString("保存股票 code="+code));
    QString sql;
    QString where = QString("code='%1'").arg(code);
    QSqlDatabase& db = threadDB.getDB();
    QSqlQuery query(db);
    if(StockMgr::exist(db, "Stocks", where))
    {
        query.prepare(QString("UPDATE Stocks SET industry=?,area=?,totals=?,outstanding=?,timeToMarket=?,minDate=?,maxDate=?,blacklist=? WHERE %1").arg(where));
        query.addBindValue(waitSave->industry);
        query.addBindValue(waitSave->area);
        query.addBindValue(waitSave->total);
        query.addBindValue(waitSave->out);
        query.addBindValue(waitSave->marketTime);
        query.addBindValue(waitSave->minDate);
        query.addBindValue(waitSave->maxDate);
        query.addBindValue(waitSave->blacklist);
        if(!query.exec())
        {
            StockMgr::single()->notifyMessage(MsgType::Error, QString("股票代号"+code+"更新记录失败!"));
            return false;
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
                %10 \
                );").arg(waitSave->code).arg(waitSave->name).arg(waitSave->industry).arg(waitSave->area).arg(waitSave->total).arg(waitSave->out).arg(waitSave->marketTime).arg(waitSave->minDate).arg(waitSave->maxDate).arg(waitSave->blacklist);
        query.prepare(sql);
        if(!query.exec())
        {
            StockMgr::single()->notifyMessage(MsgType::Error, QString("股票代号"+code+"添加记录失败!"));
            return false;
        }
    }

    if(waitSave->history.size()>0)createTable(db);
    QString tbName = "X"+code;
    for(int m=0;m<waitSave->history.size();++m)
    {//最新数据在最前面
        KData* k = waitSave->history[m];
        QString sql = QString("INSERT OR IGNORE INTO %1 (date,high,open,close,low,volume,amount) VALUES \
               (  \
               '%2', \
               %3, \
               %4, \
               %5, \
               %6, \
               %7, \
               %8  \
               );").arg(tbName).arg(k->date).arg(k->high).arg(k->open).arg(k->close).arg(k->low).arg(k->volume).arg(k->amount);
        query.prepare(sql);
        if(!query.exec())
        {
            StockMgr::single()->notifyMessage(MsgType::Error, QString("股票代号"+code+"添加历史记录失败 date="+k->date));
            return false;
        }
    }
    return true;
}

void Stock::mergeHistory(const QList<KData*>& ls)
{
    int lastMin  = minDate;
    int lastMax = maxDate;
    int head=0;
    for(int i=0;i<ls.size();++i)
    {//最新数据在最前面
        KData* k = ls[i];
        if(k->date<lastMin)
        {
            history.push_back(k);
            minDate=k->date;
            k->avg = k->amount/k->volume;
            k->turnover = (out==0?0:k->volume/out)*0.000001;
            dirty = true;
            k->dirty=true;
        }
        else if(k->date>lastMax)
        {
            history.insert(head++, k);
            if(k->date>maxDate)maxDate=k->date;
            k->avg = k->amount/k->volume;
            k->turnover = (out==0?0:k->volume/out)*0.000001;
            dirty = true;
            k->dirty=true;
        }
    }
    validHistory.clear();
}

void Stock::reset()
{
    QSqlQuery query(threadDB.getDB());
    if(!query.exec(QString("drop table  X%1").arg(code)))return;
    minDate = 88880808;
    maxDate = 0;
    history.clear();
    dirty=true;
    validHistory.clear();
}

void Stock::removeHistory(int startDate, bool removeBehind)
{
    getHistory();
    QSqlQuery query(threadDB.getDB());
    if(removeBehind)
    {
        if(!query.exec(QString("delete from X%1 where date >= %2").arg(code).arg(startDate)))return;
        for(int i=history.size()-1;i>=0;--i)if(history[i]->date>=startDate)history.removeAt(i);
        for(int i=validHistory.size()-1;i>=0;--i)if(validHistory[i]->date>=startDate)validHistory.removeAt(i);
        if(startDate<=minDate)
        {
            minDate = 88880808;
            maxDate=0;
        }
        if(history.size()>0)maxDate = history[0]->date;
    }
    else
    {
        if(!query.exec(QString("delete from X%1 where date <= %2").arg(code).arg(startDate)))return;
        for(int i=history.size()-1;i>=0;--i)if(history[i]->date<=startDate)history.removeAt(i);
        for(int i=validHistory.size()-1;i>=0;--i)if(validHistory[i]->date<=startDate)validHistory.removeAt(i);
        if(startDate>=maxDate)
        {
            minDate = 88880808;
            maxDate=0;
        }
        if(history.size()>0)minDate = history[history.size()-1]->date;
    }
    dirty=true;
}


void Stock::prepareSave()
{
    if(!dirty)return;
    if(waitSave==nullptr)waitSave=new Stock(code);
    *waitSave = *this;
    waitSave->minDate = this->minDate;
    waitSave->maxDate= this->maxDate;
    waitSave->blacklist =this->blacklist;
    for(int i=0;i<history.length();++i)
    {
         KData* k = history[i];
         if(!k->dirty)continue;
         k->dirty=false;
         waitSave->history.push_back(k);
    }
    this->dirty=false;
}

void Stock::calculate()
{//计算涨幅和均线
    const QList<KData*>& ls = getValidHistory();
    for(int i=0,count=ls.size();i<count;++i)
    {
        KData* kd = ls[i];
        //涨幅
        kd->change = (i<count-1?(kd->close-ls[i+1]->close)/ls[i+1]->close : 0)*100;

        if(i<count-5)
        {//5日
            for(int m=0;m<5;++m)
            {
                kd->ma5 +=ls[i+m]->avg;
                kd->vma5+=ls[i+m]->volume;
            }
            kd->ma5 /= 5;
            kd->vma5 /= 5;
        }

        if(i<count-10)
        {//10日
            for(int m=0;m<10;++m)
            {
                kd->ma10 +=ls[i+m]->avg;
                kd->vma10+=ls[i+m]->volume;
            }
            kd->ma10 /= 10;
            kd->vma10 /= 10;
        }

        if(i<count-20)
        {//20日
            for(int m=0;m<20;++m)
            {
                kd->ma20 +=ls[i+m]->avg;
            }
            kd->ma20 /= 20;
        }
    }
}

