#ifndef STOCK_H
#define STOCK_H
#include <QList>
#include <QSqlDatabase>
#include <QDebug>
#include "utility/RWLock.h"

enum StockState
{
    DataNotNew=0x0001,//数据不是最新
    DataLose    =0x0002,//中间数据丢失
    DataError    =0x0004,//数据有误
};

class KData
{
public:
    bool dirty;
    int    user;
    //基础数据
    int    date;//yyyyMMdd
    float high;
    float low;
    float open;
    float close;
    double volume;//交易量
    double amount;//交易额
    //衍生数据(可以通过基础数据计算得出)
    float avg;      //平均价格
    float change;//涨跌幅(100*(今收或现价-昨收)/昨收)，10%的限价就是相对昨天的收盘价的
    float turnover;//换手率(成交量/流通量)
    float ma5;     //5日均价
    float ma10;   //10日均价
    float ma20;   //20日均价
    double vma5;   //5日均量
    double vma10; //10日均量

    KData():dirty(false)
    {
        memset(this, 0, sizeof(KData));
    }
};

struct AnalyseInfo
{
    int yearUp; //本年上涨天数
    int yearDown;
    int monthUp;//本月上涨天数
    int monthDown;
    int sevenUp;//近7天上涨天数
    int sevenDown;
    int continueRiseDay;//连续上涨天数
    float continueRiseRate;//连续累计涨幅
    float volumeRate;//量比(相对昨日)
    int lookRise;//看涨，负值为看跌
    float dayResult;  //次日涨幅
    float weekResult;//周内涨幅
    KData* lastKData;
    AnalyseInfo()
    {
         memset(this, 0, sizeof(AnalyseInfo));
    }
};

struct CheckInfo
{
    int    state;
    int    firstDate;//最新的日期
    int    firstErrorDate;//检测到的第一个错误数据时间
    int    firstLoseDate;//检测到的第一个丢失数据时间
    CheckInfo()
    {
         memset(this, 0, sizeof(CheckInfo));
    }

    QString stateString()
    {
        QString s;
        if((state&StockState::DataNotNew)!=0)s.append("旧");
        if((state&StockState::DataLose)!=0)s.append("缺");
        if((state&StockState::DataError)!=0)s.append("错");
        return s;
    }
};


class StockMgr;
class Stock
{
public:
    bool  created;//历史数据表已建
    bool  loaded;//历史数据已加载
    QString code;
    QString name;
    QString area;
    QString industry;
    double  total;
    double  out;//单位亿
    int     marketTime;//yyyyMMdd
    int     minDate;//日k开始时间
    int     maxDate;//日k结束时间
    bool  blacklist;
    AnalyseInfo mAnalyseInfo;
    CheckInfo   mCheckInfo;

private:
    bool  dirty;
    Stock* waitSave;
    QList<KData*> history;
    QList<KData*> validHistory;
    void createTable(QSqlDatabase& db);
    void reqHistory();
public:
    Stock& operator=(const Stock& tmp)
    {
        if(this->name != tmp.name){this->name=tmp.name;dirty=true;}
        if(this->area != tmp.area){this->area=tmp.area;dirty=true;}
        if(this->industry != tmp.industry){this->industry=tmp.industry;dirty=true;}
        if(abs(this->total - tmp.total)>0.0001f){this->total=tmp.total;dirty=true;}
        if(abs(this->out  - tmp.out)>0.0001f){this->out=tmp.out;dirty=true;}
        if(this->marketTime != tmp.marketTime){this->marketTime=tmp.marketTime;dirty=true;}
        return *this;
    }
    Stock(QString scode)
        :created(false)
        ,loaded(false)
        ,code(scode)
        ,minDate(88880808)
        ,maxDate(0)
        ,blacklist(false)
        ,dirty(false)
        ,waitSave(nullptr)
    {}
    const QList<KData*>& getHistory();
    const QList<KData*>& getValidHistory();
    bool save();
    void reset();
    void calculate();
    void mergeHistory(const QList<KData*>& ls);
    void removeHistory(int startDate, bool removeBehind=true);
    int market(){return code.startsWith("00")||code.startsWith("30")?1:0;}
    void setDirty(){dirty=true;}
    void clearDirty(){if(waitSave!=nullptr)delete  waitSave;waitSave=nullptr;}
    void prepareSave();

    friend class  StockMgr;
};

#endif // STOCK_H
