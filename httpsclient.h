#ifndef HTTPSCLIENT_H
#define HTTPSCLIENT_H

#include <QObject>
#include <QTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>

class HttpsClient : public QObject
{
    Q_OBJECT
public:
    enum RequestType {
        GET,
        POST
    };
    explicit HttpsClient(QObject *parent = nullptr);
    int verificationCode();
    int login(const QString &name, const QString &password, const QString &verifyCode);
    int upgradePackageList(QList<QStringList> &stringList);
    int downloadPackage(const int custOtaId, QString &filename);

private:
    void networkRequest(RequestType reqType, const QString &url, QByteArray &data,
                        const QJsonDocument &document = QJsonDocument());
    QTimer *m_requestTimer = nullptr;
    QEventLoop *m_eventLoop = nullptr;
    QNetworkAccessManager *m_accessManager = nullptr;
    QString m_token;
    QString m_verifyCode;
    QByteArray m_filename;

public slots:
    void onNetRequest(QString *data);

signals:

};

#endif // HTTPSCLIENT_H
