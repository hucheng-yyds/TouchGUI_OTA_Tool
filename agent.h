#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>

class Agent : public QObject
{
    Q_OBJECT
public:
    explicit Agent(QObject *parent = nullptr);

    void startScanDevice(uint32_t timeOut, const QString &address);

private:
    void SendMessage(QString);
private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onError(QBluetoothDeviceDiscoveryAgent::Error err);
    void onFinished(void);
    void onCanceled(void);

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void message(QString msg);

private:
    QBluetoothDeviceDiscoveryAgent *m_agent;
    QString m_address;
};

#endif // AGENT_H
