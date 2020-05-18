#ifndef STOCKDATA_H
#define STOCKDATA_H

#include <QObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

class StockData : public QObject
{
    Q_OBJECT
public:
    explicit StockData(QObject *parent = nullptr);

    void ReqData(int SID);
signals:

public slots:
    void OnReqData(QNetworkReply* reply);
};

#endif // STOCKDATA_H
