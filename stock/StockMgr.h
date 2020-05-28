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
#include <sstream>
#include "stock/Stock.h"
#include "stock/StockTask.h"
using namespace  std;
enum TaskFlag
{
    InitSync,
    SyncData,
    SaveDB,
    Analyse,
    Check,
};

enum MsgType
{
    Info,   //信息日志
    Error, //错误日志
    MessageBox, //提示框
    TaskCount,//后台任务数
};

class ThreadDB
{
    //同个连接不能跨线程使用，所以使用线程局部存储，每个线程保持一个连接
    QSqlDatabase mDB;
public:
    ThreadDB()
    {
        ostringstream oss;
        oss<<std::this_thread::get_id();
        unsigned long long tid = std::stoull(oss.str());
        mDB = QSqlDatabase::addDatabase("QSQLITE", QString().sprintf("db%d",tid));//连接不能同名
        mDB.setDatabaseName("StockDB.db");
        mDB.open();
    }

    ~ThreadDB()
    {
        mDB.close();
    }

    QSqlDatabase& getDB()
    {
        return mDB;
    }
};

extern thread_local ThreadDB threadDB;
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
    QList<Stock*> mStocks;
    QMap<QString,Stock*> mStockMap;
    QThread* mThread;
    QThreadPool mPool;
    QMutex mMutex;
    QWaitCondition mSyncCompleted;
    int mTaskCount;//后台任务数
    int mMinDate;
    int mMaxDate;

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
    void notifyTaskFinished(StockTask* task){emit sendTaskFinished(QSharedPointer<StockTask>(task));}//使用共享指针保证每个槽函数调用完后再释放task
    void notifyMessage(int type, QString msg){emit sendMessage(type,msg);}
    int getTaskCount(){return mTaskCount;}
    //获取股票数据
    Stock* getStock(QString code,bool create=true);
    const QList<Stock*>& getStocks(){return mStocks;}
    //同步股票数据
    void syncData(Stock* stock=nullptr);
    void saveData();
    void checkData(Stock* stock=nullptr);
    //分析股票数据
    void analyseData(Stock* stock=nullptr);
    //是否有任务未处理完
    bool isBusy(){return mTaskCount>0||mPool.activeThreadCount()>0;}
    //判断表是否存在
    static bool exist(QSqlDatabase& db, QString table, QString where=nullptr);
    //日期转换
    static QString int2StrTime(int time){return QString::number(time).insert(6,'-').insert(4,'-');}
    static int str2IntTime(QString time){return time.remove('-').toInt();}

public slots:
    void onTaskFinished(QSharedPointer<StockTask> task);

signals:
    void sendTaskFinished(QSharedPointer<StockTask> task);
    void sendSyncProgress(float progress);
    void sendMessage(int type, QString msg);
};
#endif // ARALESTOCK_H
