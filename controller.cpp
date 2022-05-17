#include "controller.h"

Controller::Controller(QObject *parent) : QObject(parent)
{
    m_controller = NULL;
}

Controller::~Controller()
{
    if(m_controller)
    {
        m_controller->disconnectFromDevice();

        delete m_controller;

        m_controller = NULL;
    }
}

void Controller::ConnectDevice(const QBluetoothDeviceInfo &info)
{
    if(m_controller)
    {
        m_controller->disconnectFromDevice();

        delete m_controller;

        m_controller = NULL;
    }
    qDebug() << "ConnectDevice:" << info.name();

    m_controller = QLowEnergyController::createCentral(info);
    connect(m_controller, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_controller, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(m_controller, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(onStateChanged(QLowEnergyController::ControllerState)));
    connect(m_controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onError(QLowEnergyController::Error)));

    connect(m_controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(onServiceDiscovered(QBluetoothUuid)));
    connect(m_controller, SIGNAL(discoveryFinished()), this, SLOT(onDiscoveryFinished()));
    connect(m_controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(onConnectionUpdated(QLowEnergyConnectionParameters)));

    m_controller->connectToDevice();
}

void Controller::DisconnectDevice()
{
    if(m_controller)
    {
        m_controller->disconnectFromDevice();
    }
}

QLowEnergyService * Controller::CreateService(QBluetoothUuid serviceUUID)
{
    if(m_controller)
    {
        return m_controller->createServiceObject(serviceUUID);
    }
    else
    {
        return NULL;
    }
}

void Controller::SendMessage(QString str)
{
    qDebug() << "Controller:" << str;
    emit message(str);
}

void Controller::onConnected()
{
    if(m_controller)
    {
        m_controller->discoverServices();

        SendMessage("device connected");
    }
}

void Controller::onDisconnected()
{
    SendMessage("device disconnected");
}

void Controller::onStateChanged(QLowEnergyController::ControllerState state)
{
    const QString stateString[] = {
        "UnconnectedState",
        "ConnectingState",
        "ConnectedState",
        "DiscoveringState",
        "DiscoveredState",
        "ClosingState",
        "AdvertisingState",
    };

    if(state < stateString->size())
    {
        SendMessage(stateString[state]);
    }
}

void Controller::onError(QLowEnergyController::Error newError)
{
    if(m_controller)
    {
        QString str;

        str = QString("Controller Error(%1):").arg(newError);
        str += m_controller->errorString();

        SendMessage(str);
    }
}

void Controller::onServiceDiscovered(QBluetoothUuid serviceUUID)
{
    if (0x27f0 == serviceUUID.data1
            || 0x0af0 == serviceUUID.data1) {
        qDebug() << "serviceUUID:" << serviceUUID << serviceUUID.data1;
        emit serviceDiscovered(serviceUUID);
    }
}

void Controller::onDiscoveryFinished()
{
    SendMessage("service discovery finished");
}

void Controller::onConnectionUpdated(const QLowEnergyConnectionParameters &parameters)
{
    Q_UNUSED(parameters);
    SendMessage("controller connect updated");
}
