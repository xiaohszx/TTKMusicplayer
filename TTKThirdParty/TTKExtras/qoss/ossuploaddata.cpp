#include "ossuploaddata.h"
#include "ossdatainterface_p.h"

#include <QFile>

class OSSUploadDataPrivate : public OSSDataInterfacePrivate
{
public:
    OSSUploadDataPrivate() : OSSDataInterfacePrivate()
    {
    }

    QString m_uploadTime;
};



OSSUploadData::OSSUploadData(QNetworkAccessManager *networkManager, QObject *parent)
    : OSSDataInterface(networkManager, parent)
{
    TTK_INIT_PUBLIC(OSSUploadData);
    TTK_INIT_PRIVATE;
    TTK_D(OSSUploadData);
    d->m_networkManager = networkManager;
}

void OSSUploadData::uploadDataOperator(const QString &time, const QString &bucket, const QString &fileName, const QString &filePath)
{
    TTK_D(OSSUploadData);
    d->m_uploadTime = time;

    const QString &method = "PUT";
    const QString &url = "/" + fileName;
    const QString &resource = "/" + bucket + "/" + fileName;
    const QString &host = bucket + "." + OSSConf::OSS_HOST;

    MStringMap headers;
    headers.insert("Date", OSSUtils::getGMT());
    headers.insert("Host", host);
    headers.insert("Content-Type", "charset=utf-8");

    d->insertAuthorization(method, headers, resource);

    QNetworkRequest request;
    request.setUrl(QUrl("http://" + host + url));

    MStringMapterator it(headers);
    while(it.hasNext())
    {
        it.next();
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QFile file(filePath);
    file.open(QFile::ReadOnly);
    QByteArray fileData = file.readAll();
    file.close();

    QNetworkReply *reply = d->m_networkManager->put(request, fileData);
    connect(reply, SIGNAL(finished()), SLOT(receiveDataFromServer()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleError(QNetworkReply::NetworkError)));
    if(parent()->metaObject()->indexOfSlot("uploadProgress(QString,qint64,qint64)") != -1)
    {
        connect(reply, SIGNAL(uploadProgress(qint64,qint64)), SLOT(uploadProgress(qint64,qint64)));
        connect(this, SIGNAL(uploadProgressChanged(QString,qint64,qint64)), parent(), SLOT(uploadProgress(QString,qint64,qint64)));
    }
}

void OSSUploadData::receiveDataFromServer()
{
    TTK_D(OSSUploadData);
    QNetworkReply *reply = MObject_cast(QNetworkReply*, QObject::sender());
    if(reply)
    {
        if(reply->error() == QNetworkReply::NoError)
        {
            Q_EMIT uploadFileFinished(d->m_uploadTime);
        }
        else
        {
            Q_EMIT uploadFileFinished(TTK_NAN_STR);
        }
        reply->deleteLater();
    }
    else
    {
        Q_EMIT uploadFileFinished(TTK_NAN_STR);
    }
}

void OSSUploadData::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    TTK_D(OSSUploadData);
    Q_EMIT uploadProgressChanged(d->m_uploadTime, bytesSent, bytesTotal);
}
