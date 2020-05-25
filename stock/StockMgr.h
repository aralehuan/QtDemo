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
#include <QWaitCondition>

enum TaskFlag
{
    InitSync,
    SyncData,
    SaveDB,
};

enum MsgType
{
    Info,
    Error,
    TaskCount,
};

class KData
{
public:
    bool dirty;
    int    date;//yyyyMMdd
    float high;
    float low;
    float open;
    float close;
    double volume;
    float ma5;

    KData():dirty(false){}
};

class StockMgr;
class Stock
{
public:
    bool  dirty;
    bool  created;//历史数据表已建
    bool  loaded;//历史数据已加载
    QString code;
    QString name;
    QString area;
    QString industry;
    double  total;
    double  out;
    int     marketTime;//yyyyMMdd
    int     minDate;//日k开始时间
    int     maxDate;//日k结束时间

private:
    QList<KData*> history;
    void createTable(QSqlDatabase& db);
    void reqHistory();

public:
    Stock& operator=(const Stock& tmp)
    {
        if(this->code != tmp.code){this->code=tmp.code;dirty=true;}
        if(this->name != tmp.name){this->name=tmp.name;dirty=true;}
        if(this->area != tmp.area){this->area=tmp.area;dirty=true;}
        if(this->industry != tmp.industry){this->industry=tmp.industry;dirty=true;}
        if(this->total != tmp.total){this->total=tmp.total;dirty=true;}
        if(this->out != tmp.out){this->out=tmp.out;dirty=true;}
        return *this;
    }
    Stock(QString scode):dirty(false),created(false),loaded(false),code(scode),minDate(88880808),maxDate(0){}
    const QList<KData*>& getHistory();
    void mergeHistory(const QList<KData*>& ls);

    friend class  StockMgr;
};

class StockTask : public QRunnable
{
public:
    Stock*  mStock;
    int   mTaskFlag;

    StockTask(Stock* s,int flag):mStock(s),mTaskFlag(flag)
    {
        setAutoDelete(false);//不设置，线程会自动释放掉该runable
    }

    virtual ~StockTask(){}
    virtual void onFinished(){};
};

class PullHistoryTask : public StockTask
{
protected:
    QList<KData*> cach;
    int mBeginTime;
    int mEndTime;
public:
    PullHistoryTask(Stock* s, int flag, int beginTime, int endTime):StockTask(s,flag),mBeginTime(beginTime),mEndTime(endTime){}
    void run();
    void onFinished();
};

class PullStocksTask : public StockTask
{
protected:
    QList<Stock> cach;
public:
    PullStocksTask(int flag):StockTask(nullptr,flag){}
    void run();
    void onFinished();
};

class SaveDBTask : public StockTask
{
protected:
    QList<Stock> cach;
public:
    SaveDBTask(int flag):StockTask(nullptr,flag){}
    void run();
    void onFinished(){};
};

class StockMgr : public QObject
{
    Q_OBJECT

    //自动同步最多同步3年前数据
    const int OLDEST_YEAR_DAY=20170101;

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
    QThread* mThread;
    QThreadPool mPool;
    QMutex mMutex;
    QWaitCondition mSyncCompleted;
    int mTaskCount;//后台任务数
    bool mSaving;

   //创建股票信息表
   QString createTable(QSqlDatabase& db);
   //加载股票列表
   void loadStocks(QSqlDatabase& db);
   //获取kdata有效时间(避免每次都请求所有股票)
   void getValideDate(int& minDate, int& maxDate);
   //=============
   void threadWait()
   {
       mMutex.lock();
       mSyncCompleted.wait(&mMutex);
       mMutex.unlock();
   }
   void threadWake()
   {
       mMutex.lock();
       mSyncCompleted.wakeAll();
       mMutex.unlock();
   }

public:
    QString init();
    void deinit();
    //后台任务
    void startTask(QRunnable* task){++mTaskCount;mPool.start(task); emit sendMessage(TaskCount,nullptr);}
    void notifyTaskFinished(StockTask* task){emit sendTaskFinished(task);}
    int getTaskCount(){return mTaskCount;}
    //获取股票数据
    Stock* getStock(QString code,bool create=true);
    const QList<Stock*>& getStocks(){return mStocks;}
    //同步股票数据
    void syncData();
    void saveData();
    //判断表是否存在
    static bool exist(QSqlDatabase& db, QString table, QString where=nullptr);
    //日期转换
    static QString int2StrTime(int time){return QString::number(time).insert(6,'-').insert(4,'-');}
    static int str2IntTime(QString time){return time.remove('-').toInt();}

public slots:
    void onTaskFinished(StockTask* task);

signals:
    void sendTaskFinished(StockTask* task);
    void sendSyncProgress(float progress);
    void sendMessage(int type, QString msg);
};
#endif // ARALESTOCK_H
