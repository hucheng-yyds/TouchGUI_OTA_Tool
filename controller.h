#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QThread>
#include <QLowEnergyController>
#include <QBluetoothDeviceInfo>
#include "service.h"

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    void SetProperty(QByteArrayList &data, QByteArrayList &name, int size);
    void ConnectDevice(const QBluetoothDeviceInfo &info);
    void DisconnectDevice(void);

    QLowEnergyService *CreateService(QBluetoothUuid);

private:
    void SendMessage(QString);

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(QLowEnergyController::ControllerState state);
    void onError(QLowEnergyController::Error newError);

    void onServiceDiscovered(QBluetoothUuid);
    void onDiscoveryFinished();
    void onConnectionUpdated(const QLowEnergyConnectionParameters &parameters);
    void onReconnectDevice();

signals:
    void message(QString msg);;
    void serviceDiscovered(QLowEnergyService *service, const QString &address);
    void upgradeResult(bool success, const QString &address);

private:
    QThread *m_thread = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QBluetoothDeviceInfo m_device_info;
    Service *m_service = nullptr;
};

#endif // CONTROLLER_H
