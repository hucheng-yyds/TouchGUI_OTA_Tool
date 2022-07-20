#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QLowEnergyController>
#include <QBluetoothDeviceInfo>
#include "service.h"
#include <QTimer>

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    void SetProperty(QByteArrayList &data, QByteArrayList &name, int size, QByteArray &version);
    void ConnectDevice(const QBluetoothDeviceInfo &info, int timeout);
    void DisconnectDevice(void);

    QLowEnergyService *CreateService(QBluetoothUuid);

    void setIgnoreVersionCompare();
    void setOTAPoweroff();

private:
    void SendMessage(const QString &);

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(QLowEnergyController::ControllerState state);
    void onError(QLowEnergyController::Error newError);

    void onServiceDiscovered(QBluetoothUuid);
    void onDiscoveryFinished();
    void onConnectionUpdated(const QLowEnergyConnectionParameters &parameters);
    void onReconnectDevice();
    void onStartOTA();
    void onStartOTATimeout();
    void onStartError();

signals:
    void message(QString msg);
    void serviceDiscovered(QLowEnergyService *service, const QString &address);
    void upgradeResult(bool success, const QString &address);
    void startError();

private:
    void deviceError();

private:
    QThread *m_thread = nullptr;
    //QLowEnergyController *m_controller = nullptr;
    QPointer<QLowEnergyController> m_controller;
    QBluetoothDeviceInfo m_device_info;
    Service *m_service = nullptr;
    int m_timeout_count = 3;

    bool m_startOTA = false;
    bool m_startError = false;

    QPointer<QTimer> m_startOTATimer;
};

#endif // CONTROLLER_H
