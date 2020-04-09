#ifndef ARALESTOCK_H
#define ARALESTOCK_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QList>
#include <QThread>

class KData
{
public:
    QString date;
    float high;
    float low;
    float open;
    float close;
    double volume;
    float ma5;
};

class Stock
{
public:
    QString code;
    QString name;
    QString area;
    QString industry;
    float   total;
    float   out;
    int     marketTime;
    QList<KData*> history;
};

class AraleStock : public QObject
{
private:
    QSqlDatabase mDB;
    QList<Stock*> mStocks;
    QThread mThread;
public:
    AraleStock();
    const QList<Stock*>& getStocks(){return mStocks;}
    QString init();
    void deinit();
    QString loadStocks();
    QString loadKData(Stock* stock);
    QString reqStocks();
    QString reqStock(QString code);
    bool exist(QString table, QString where=nullptr);
    QString createStockTable();
    QString createStockTable(QString code);
public slots:
    void onRefresh();
};

#endif // ARALESTOCK_H
