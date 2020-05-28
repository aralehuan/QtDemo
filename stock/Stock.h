#ifndef STOCK_H
#define STOCK_H
#include <QList>
#include <QSqlDatabase>
#include <QDebug>

class KData
{
public:
    bool dirty;
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
    double vma20; //20日均量

    KData():dirty(false)
    {
        memset(this, 0, sizeof(KData));
    }
};

struct AnalyseInfo
{
    int yearUp;
    int yearDown;
    int monthUp;
    int monthDown;
    AnalyseInfo()
    {
         memset(this, 0, sizeof(AnalyseInfo));
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
    double  out;
    int     marketTime;//yyyyMMdd
    int     minDate;//日k开始时间
    int     maxDate;//日k结束时间
    bool  blacklist;
    AnalyseInfo mAnalyseInfo;

private:
    bool  dirty;
    QList<KData*> history;
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
    {}
    const QList<KData*>& getHistory();
    bool save();
    void reset();
    void removeHistory(int beginDate, int endDate);
    void mergeHistory(const QList<KData*>& ls);
    int market(){return code.startsWith("00")||code.startsWith("30")?1:0;}
    void setDirty(){dirty=true;}
    void clearDirty(){dirty=false;for(int i=0;i<history.size();++i)history[i]->dirty=false;}

    friend class  StockMgr;
};

#endif // STOCK_H
