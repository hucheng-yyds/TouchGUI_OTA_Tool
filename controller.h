#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QLowEnergyController>
#include <QBluetoothDeviceInfo>

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

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

signals:
    void message(QString msg);
    void serviceDiscovered(const QBluetoothUuid &newService);

private:
    QLowEnergyController *m_controller;

};

#endif // CONTROLLER_H
