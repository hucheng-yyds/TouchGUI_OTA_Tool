#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QTimer>

class Agent : public QObject
{
    Q_OBJECT
public:
    explicit Agent(QObject *parent = nullptr);
    bool isActive();
    void setMatchStr(const QString &matchStr);
    bool isFindCountEnough() const;
    void startScan();
    void stopScan();
    void cancelScan();
    void startTimer();
    void stopTimer();
    void resetData();

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onError(QBluetoothDeviceDiscoveryAgent::Error err);
    void onFinished(void);
    void onCanceled(void);

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void deviceListDiscovered(const QList<QBluetoothDeviceInfo> &infoList);
    void scanFinished(bool isTimeout);

private:
    QBluetoothDeviceDiscoveryAgent *m_agent;
    int m_find_count = 0;
    bool m_not_find = true;
    bool m_cancel = false;
    QTimer *m_timer;
    QString m_match_str;
};

#endif // AGENT_H
