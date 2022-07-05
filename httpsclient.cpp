#include "httpsclient.h"
#include <QDebug>
#include <QNetworkReply>
#include <QFile>
#include <QJsonArray>

#if 1
#define SERVER_ADDRESS  "http://47.243.45.185:8966/"
#else
#define SERVER_ADDRESS  "http://192.168.20.134:8080/"
#endif

HttpsClient::HttpsClient(QObject *parent)
    : QObject{parent}
    , m_requestTimer(new QTimer(this))
    , m_eventLoop(new QEventLoop(this))
    , m_accessManager(new QNetworkAccessManager(this))
{
    m_requestTimer->setInterval(15 * 1000);
    m_requestTimer->setSingleShot(true);
}

int HttpsClient::verificationCode()
{
    QByteArray data;
    const QString &url = "touchlink/verification/ordinary";
    networkRequest(GET, SERVER_ADDRESS + url, data);
    if (data.isEmpty()) {
        return -1;
    }
    QFile file("a.jpg");
    file.open(QIODevice::ReadWrite);
    file.write(data);
    file.close();
    return 0;
}

int HttpsClient::login(const QString &name, const QString &password, const QString &verifyCode)
{
    QJsonDocument document;
    QJsonObject jsonObj;
    jsonObj.insert("name", name);
    jsonObj.insert("password", password);
    m_verifyCode = verifyCode;
    document.setObject(jsonObj);
    QByteArray data;
    const QString &url = "touchlink/customer/ota/tool/login";
    networkRequest(POST, SERVER_ADDRESS + url, data, document);
    QJsonParseError jsonError;
    document = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        jsonObj = document.object();
        qDebug() << jsonObj;
        if (jsonObj.contains("code")
                && 200 == jsonObj["code"].toInt()) {
            m_token = jsonObj["data"].toObject()["token"].toString();
            return 0;
        }
    }
    return -1;
}

int HttpsClient::upgradePackageList(QList<QStringList> &stringList)
{
    QJsonDocument document;
    QJsonObject jsonObj;
    jsonObj.insert("current", 0);
    jsonObj.insert("size", 0);
    document.setObject(jsonObj);
    QByteArray data;
    const QString &url = "touchlink/customer/ota/tool/list";
    networkRequest(POST, SERVER_ADDRESS + url, data, document);
    QJsonParseError jsonError;
    document = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        jsonObj = document.object();
        qDebug() << jsonObj;
        if (jsonObj.contains("code")
                && 200 == jsonObj["code"].toInt()) {
            QJsonArray array = jsonObj["data"].toObject()["records"].toArray();
            for (const auto &value : qAsConst(array)) {
                QStringList list;
                list << QString::number(value.toObject()["custOtaId"].toInt());
                list << value.toObject()["custOtaTargetVersion"].toString();
                list << value.toObject()["custProductCenterName"].toString();
                list << value.toObject()["createdTime"].toString();
                stringList << list;
            }
            return 0;
        }
    } else {
        qDebug() << "jsonError:" << jsonError.error;
    }
    return -1;
}

int HttpsClient::downloadPackage(const int custOtaId)
{
    QByteArray data;
    const QString &url = "touchlink/customer/ota/tool/downloadOTA/" + QString::number(custOtaId);
    networkRequest(GET, SERVER_ADDRESS + url, data);
    if (data.isEmpty()) {
        return -1;
    }
    if (m_filename.isEmpty()) {
        m_filename = "ota.zip";
    }
    QFile file(m_filename);
    file.open(QIODevice::ReadWrite);
    file.write(data);
    file.close();
    QByteArrayList dirname = m_filename.split('.');
    system("mkdir " + dirname.value(0)); // del /f /s /q  ota\*
    system("tar -xvf " + m_filename + " -C %cd%/" + dirname.value(0));
    return 0;
}

void HttpsClient::networkRequest(RequestType reqType, const QString &url,
                                 QByteArray &data, const QJsonDocument &document)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader , "application/json");
    if (m_token.isEmpty()) {
        request.setRawHeader("ordinaryVerifyCode", m_verifyCode.toUtf8());
    } else {
        request.setRawHeader("Authorization_Token", m_token.toUtf8());
    }

    QNetworkReply *reply = nullptr;
    if (GET == reqType) {
        reply = m_accessManager->get(request);
    } else {
        reply = m_accessManager->post(request, document.toJson());
    }

    connect(reply, &QNetworkReply::finished, m_eventLoop, &QEventLoop::quit);
    connect(m_requestTimer, &QTimer::timeout, m_eventLoop, &QEventLoop::quit);
    m_requestTimer->start();
    m_eventLoop->exec(QEventLoop::WaitForMoreEvents);
    if (m_requestTimer->isActive()) {
        m_requestTimer->stop();
        if (reply->error() == QNetworkReply::NoError) {
            data = reply->readAll();
            qDebug() << "request post success";
            for (auto &header : reply->rawHeaderList()) {
                qDebug() << header << reply->rawHeader(header);
                if ("Content-Disposition" == header) {
                    QByteArrayList list = reply->rawHeader(header).split('=');
                    m_filename = list.value(1);
                    qDebug() << "m_filename:" << m_filename;
                }
            }
        } else {
            QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            qDebug() << "status code: " << statusCodeV.toInt();
            qDebug() << "status code: " << (int)reply->error();
        }
    } else {
        disconnect(reply, &QNetworkReply::finished, m_eventLoop, &QEventLoop::quit);
        disconnect(m_requestTimer, &QTimer::timeout, m_eventLoop, &QEventLoop::quit);
        reply->abort();
        qDebug() << "request time out";
    }
    reply->deleteLater();
}

void HttpsClient::onNetRequest(QString *data)
{
    qDebug() << data << *data;
}
