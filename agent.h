#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QTimer>
#include <QPointer>

class Agent : public QObject
{
    Q_OBJECT
public:
    explicit Agent(QObject *parent = nullptr);

    void startScanDevice(uint32_t timeOut, const QStringList &address);
    void stopScan();
    bool isActive();
    void setMatchStr(const QString &matchStr);
    void setTargetCount(int targetcount){m_targetcount=targetcount;}
    void setSuccessCount(int successcount){m_successcount=successcount;}
    void increaseSuccessCount(const QString & succ_address);
    void restartScan(int secTimeout);

private:
    void SendMessage(const QString &);
private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onError(QBluetoothDeviceDiscoveryAgent::Error err);
    void onFinished(void);
    void onCanceled(void);

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void scanFinished();
    void message(QString msg);

private:
    //QBluetoothDeviceDiscoveryAgent *m_agent;
    QPointer<QBluetoothDeviceDiscoveryAgent> m_agent;
    QStringList m_address_list;
    int m_find_count = 0;
    bool m_not_find = true;
    //QTimer *m_timer = nullptr;
    QPointer<QTimer> m_timer;
    QString m_match_str;

    int m_targetcount = 0;
    int m_successcount = 0;
};

#endif // AGENT_H
