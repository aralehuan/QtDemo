#pragma execution_character_set("utf-8")
#include "Stock.h"
#include "StockMgr.h"

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
         k->turnover = out==0?0:k->volume/out;
         history.append(k);
         if(k->date>maxDate)maxDate=k->date;
         if(k->date<minDate)minDate=k->date;
     }
     std::sort(history.begin(),history.end(),[](const KData* a,const KData* b){return a->date>b->date;});
     loaded=true;
    return history;
}

bool Stock::save()
{
    if(!dirty)return true;
    StockMgr::single()->sendMessage(MsgType::Info, QString("保存股票 code="+code));
    QString sql;
    QString where = QString("code='%1'").arg(code);
    QSqlDatabase& db = threadDB.getDB();
    QSqlQuery query(db);
    if(StockMgr::exist(db, "Stocks", where))
    {
        query.prepare(QString("UPDATE Stocks SET industry=?,area=?,totals=?,outstanding=?,timeToMarket=?,minDate=?,maxDate=?,blacklist=? WHERE %1").arg(where));
        query.addBindValue(industry);
        query.addBindValue(area);
        query.addBindValue(total);
        query.addBindValue(out);
        query.addBindValue(marketTime);
        query.addBindValue(minDate);
        query.addBindValue(maxDate);
        query.addBindValue(blacklist);
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
                );").arg(code).arg(name).arg(industry).arg(area).arg(total).arg(out).arg(marketTime).arg(minDate).arg(maxDate).arg(blacklist);
        query.prepare(sql);
        if(!query.exec())
        {
            StockMgr::single()->notifyMessage(MsgType::Error, QString("股票代号"+code+"添加记录失败!"));
            return false;
        }
    }

    if(history.size()>0)createTable(db);
    QString tbName = "X"+code;
    for(int m=0;m<history.size();++m)
    {//最新数据在最前面
        KData* k = history[m];
        if(!k->dirty)continue;
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
            k->turnover = out==0?0:k->volume/out;
            dirty = true;
            k->dirty=true;
        }
        else if(k->date>lastMax)
        {
            history.insert(head++, k);
            if(k->date>maxDate)maxDate=k->date;
            k->avg = k->amount/k->volume;
            k->turnover = out==0?0:k->volume/out;
            dirty = true;
            k->dirty=true;
        }
    }
}

void Stock::reset()
{
    QSqlQuery query(threadDB.getDB());
    if(!query.exec(QString("drop table  X%1").arg(code)))return;
    minDate = 88880808;
    maxDate = 0;
    history.clear();
    dirty=true;
}

void Stock::removeHistory(int beginDate, int endDate)
{
    QSqlQuery query(threadDB.getDB());
    if(!query.exec(QString("delete from X%1 where date >= %2 and date <= %3").arg(code).arg(beginDate).arg(endDate)))return;
    getHistory();
    for(int i=history.size()-1;i>=0;--i)
    {
        if(history[i]->date>=beginDate&&history[i]->date<=endDate)history.removeAt(i);
    }
    minDate = 88880808;
    maxDate = 0;
    for(int i=0;i<history.size();++i)
    {
        KData* k = history[i];
        if(k->date>maxDate)maxDate=k->date;
        if(k->date<minDate)minDate=k->date;
    }
    dirty=true;
}

