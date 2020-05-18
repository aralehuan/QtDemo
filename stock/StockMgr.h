#ifndef ARALESTOCK_H
#define ARALESTOCK_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QList>
#include <QThread>
#include <QThreadPool>
#include <QtDebug>
#include <QDate>
#include <QMutex>
#include <QMap>

class KData
{
public:
    int    date;//yyyyMMdd
    float high;
    float low;
    float open;
    float close;
    double volume;
    float ma5;

    QString strDate()
    {
        return QString::number(date).insert(6,'-').insert(4,'-');
    }
};

class Stock :  public QRunnable
{
    QList<KData*> history;
    QList<KData*> cach;
    QMutex lock;

    bool dirty;
    int    minDate;
    int    maxDate;

    void reqHistory();
    void createTable();
public:
    QString code;
    QString name;
    QString area;
    QString industry;
    float   total;
    float   out;
    int     marketTime;//yyyyMMdd

public:
    Stock(QString scode):code(scode),dirty(false),minDate(88880808),maxDate(0){}
    const QList<KData*>& getHistory();
    void run();
};

class StockMgr : public QObject
{
    Q_OBJECT

    static StockMgr* mThis;
public:
    static StockMgr* single()
    {
        if(mThis!=nullptr)return mThis;
        mThis = new StockMgr();
        return mThis;
    }
private:
    StockMgr();
    QSqlDatabase mDB;
    QList<Stock*> mStocks;
    QMap<QString,Stock*> mStockMap;
    QThread mThread;
    QThreadPool mPool;

    //加载股票列表
    QString loadStocks();
    //请求股票列表数据
   QString reqStocks();
   //创建股票信息表
   QString createTable();
public:
    QString init();
    void deinit();
    QSqlDatabase& getDB(){return mDB;}
    Stock* getStock(QString code,bool create=true);
    void startTask(QRunnable* task){mPool.start(task);}
    //获取股票列表数据
    const QList<Stock*>& getStocks();
    //判断表是否存在
    static bool exist(QString table, QString where=nullptr);
    void notifyStockChanged(Stock* s){emit sendStockChanged(s);}
public slots:
    void onRefresh(){}
    void onStockChanged(Stock* s){s->getHistory();}
signals:
    void sendStockChanged(Stock* s);
};

#endif // ARALESTOCK_H
