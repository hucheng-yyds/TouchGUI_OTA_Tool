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
    explicit Controller(const QBluetoothDeviceInfo &info);
    ~Controller();

    void ConnectDevice(int timeout);
    void DisconnectDevice(void);
    QLowEnergyService *CreateService(const QBluetoothUuid &uuid);
    int connectCount() const {
        return m_connect_count;
    }
    QBluetoothDeviceInfo device() const {
        return m_device;
    }
    QString address() const {
        return m_device.address().toString();
    }

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(QLowEnergyController::ControllerState state);
    void onError(QLowEnergyController::Error newError);

    void onConnectToDevice(int timeout);
    void onServiceDiscovered(QBluetoothUuid);
    void onDiscoveryFinished();
    void onConnectionUpdated(const QLowEnergyConnectionParameters &parameters);
    void onStartOTA();
    void onStartOTATimeout();

signals:
    void connectToDevice(int timeout);
    void serviceDiscovered(QLowEnergyService *service, const QString &address);
    void upgradeResult(bool success, const QString &address);

private:
    QThread *m_thread = nullptr;
    QLowEnergyController *m_controller = nullptr;
    Service *m_service = nullptr;
    int m_connect_count = 3;
    QTimer *m_startOTATimer = nullptr;
    QBluetoothDeviceInfo m_device;
};

#endif // CONTROLLER_H
