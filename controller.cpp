#include "controller.h"
#include <QTime>
#include <QTimer>

#define print  qInfo() << m_device.address().toString()// << QThread::currentThreadId()

Controller::Controller(const QBluetoothDeviceInfo &info)
{
    m_device = info;
    m_thread = new QThread;
    moveToThread(m_thread);
    connect(this, &Controller::connectToDevice, this, &Controller::onConnectToDevice);

    m_service = new Service;
    m_service->moveToThread(m_thread);
    connect(this, &Controller::serviceDiscovered, m_service, &Service::ConnectService);
    connect(m_service, &Service::disconnectDevice, this, &Controller::DisconnectDevice);
    connect(m_service, &Service::upgradeResult, this, &Controller::upgradeResult);
    connect(m_service, &Service::startOTA, this, &Controller::onStartOTA);

    m_startOTATimer = new QTimer;
    m_startOTATimer->moveToThread(m_thread);
    m_startOTATimer->setSingleShot(true);
    connect(m_startOTATimer, &QTimer::timeout, this, &Controller::onStartOTATimeout);
}

Controller::~Controller()
{
    m_controller->disconnectFromDevice();
    m_controller->deleteLater();
    m_service->deleteLater();
    m_startOTATimer->deleteLater();
    m_thread->quit();
    qInfo() << "m_thread wait:" << m_thread->wait(30 * 1000);
    delete m_thread;
    qInfo() << "~Controller";
}

void Controller::ConnectDevice(int timeout)
{
    m_thread->start();
    emit connectToDevice(timeout);
}

void Controller::DisconnectDevice()
{
    m_controller->disconnectFromDevice();
//    m_controller->deleteLater();
//    m_controller = nullptr;
//    m_thread->quit();
    emit upgradeResult(false, address());
    print << "m_connect_count:" << m_connect_count;
}

QLowEnergyService * Controller::CreateService(const QBluetoothUuid &uuid)
{
    if (m_controller)
    {
        return m_controller->createServiceObject(uuid);
    }
    else
    {
        return NULL;
    }
}

void Controller::onConnected()
{
    if (m_controller)
    {
        m_controller->discoverServices();
    }
}

void Controller::onDisconnected()
{

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
        print << stateString[state];
    }
}

void Controller::onError(QLowEnergyController::Error newError)
{
    if(m_controller)
    {
        QString str;

        str = QString("Controller Error(%1):").arg(newError);
        str += m_controller->errorString();

        print << str;
        DisconnectDevice();
    }
}

void Controller::onConnectToDevice(int timeout)
{
    m_connect_count --;
    m_startOTATimer->start(timeout * 1000);
    if (!m_controller) {
        m_controller = QLowEnergyController::createCentral(m_device);
        m_controller->moveToThread(m_thread);
        connect(m_controller, SIGNAL(connected()), this, SLOT(onConnected()));
        connect(m_controller, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
        connect(m_controller, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(onStateChanged(QLowEnergyController::ControllerState)));
        connect(m_controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onError(QLowEnergyController::Error)));
        connect(m_controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(onServiceDiscovered(QBluetoothUuid)));
        connect(m_controller, SIGNAL(discoveryFinished()), this, SLOT(onDiscoveryFinished()));
        connect(m_controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(onConnectionUpdated(QLowEnergyConnectionParameters)));
    }
    m_controller->connectToDevice();
    print << "m_startOTATimer:" << m_startOTATimer->isActive();
}

void Controller::onServiceDiscovered(QBluetoothUuid serviceUUID)
{
//    if (0x27f0 == serviceUUID.data1
//            || 0x0af0 == serviceUUID.data1) {
//        QLowEnergyService *service= CreateService(serviceUUID);
//        emit serviceDiscovered(service, m_controller->remoteAddress().toString());
//    }
}

void Controller::onDiscoveryFinished()
{
    print << "service discovery finished";
    QList<QBluetoothUuid> uuids= m_controller->services();
    for (const auto &uuid : qAsConst(uuids)) {
        if (0x27f0 == uuid.data1
                || 0x0af0 == uuid.data1) {
            QLowEnergyService *service = CreateService(uuid);
            emit serviceDiscovered(service, address());
        }
    }
}

void Controller::onConnectionUpdated(const QLowEnergyConnectionParameters &parameters)
{
    Q_UNUSED(parameters);
    print << "controller connect updated";
}

void Controller::onStartOTA()
{
    print << "service begin to ota";
    m_startOTATimer->stop();
}

void Controller::onStartOTATimeout()
{
    print << "start ota timeout";
    DisconnectDevice();
}
