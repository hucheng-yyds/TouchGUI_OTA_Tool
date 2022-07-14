#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QTimer>
#include <QPointer>
#include <QMutex>
#include <QThread>

class Agent : public QObject
{
    Q_OBJECT
public:
    explicit Agent(QObject *parent = nullptr);
    ~Agent();
    void startScanDevice(uint32_t timeOut, const QStringList &address);
    bool isActive();
    void setMatchStr(const QString &matchStr);
    void setTargetCount(int targetcount){m_targetcount=targetcount;}
    void setSuccessCount(int successcount){m_successcount=successcount;}
    void increaseSuccessCount(const QString & succ_address);
    bool isFindCountEnough() const {return m_find_count < 6;}//没有找到一个设备算一次消耗，找到设备消耗重置为0
    void initScanData(int msTimeout, const QStringList &address);
    void setProcessingCount(int count){m_processingcount = count;}

private:
    void SendMessage(const QString &);
    void stopScan();
    void stopTimer();
    void startTimer();

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onError(QBluetoothDeviceDiscoveryAgent::Error err);
    void onFinished(void);
    void onCanceled(void);

private slots:
    void onStopAgentScan();
    void onStartAgentScan();

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void scanFinished();
    void message(QString msg);
    void stopAgentScan();
    void startAgentScan();

private:
    //QBluetoothDeviceDiscoveryAgent *m_agent;
    QPointer<QBluetoothDeviceDiscoveryAgent> m_agent;

    QMutex m_address_mutex;
    QStringList m_address_list;
    int m_find_count = 0;
    bool m_not_find = true;
    QPointer<QTimer> m_timer;
    QString m_match_str;

    int m_targetcount = 0;
    int m_successcount = 0;
    int m_processingcount = 0;
};

#endif // AGENT_H
