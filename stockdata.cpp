#include "stockdata.h"
StockData::StockData(QObject *parent) : QObject(parent)
{

}

void StockData::ReqData(int SID)
{
    QNetworkRequest req;
    QNetworkAccessManager* mgr = new QNetworkAccessManager(this);
    QMetaObject::Connection connRet = QObject::connect(mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(OnReqData(QNetworkReply*)));
    req.setUrl(QUrl("http://www.baidu.com"));
    QNetworkReply* reply = mgr->get(req);
}

void StockData::OnReqData(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode.isValid())
        qDebug() << "status code=" << statusCode.toInt();

    QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    if(reason.isValid())
        qDebug() << "reason=" << reason.toString();

    QNetworkReply::NetworkError err = reply->error();
    if(err != QNetworkReply::NoError)
    {
        qDebug() << "Failed: " << reply->errorString();
    }
    else
    {
        qDebug() << reply->readAll();
    }
}
