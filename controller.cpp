#include "controller.h"
#include <QTime>
#include <QTimer>

Controller::Controller(QObject *parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_service = new Service;
    moveToThread(m_thread);
    m_service->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_service, &Service::deleteLater);
    connect(this, &Controller::serviceDiscovered, m_service, &Service::ConnectService);
    connect(m_service, &Service::disconnectDevice, this, &Controller::DisconnectDevice);
    connect(m_service, &Service::upgradeResult, this, &Controller::upgradeResult);
    connect(m_service, &Service::startOTA, this, &Controller::onStartOTA);
    connect(this, &Controller::startError, this, &Controller::onStartError);
    m_thread->start();
}

Controller::~Controller()
{
    m_thread->quit();
    m_thread->wait();
    m_thread = nullptr;
    //delete m_thread;
    //delete m_service;
    m_service = nullptr;
    if(m_controller)
    {
        m_controller->disconnectFromDevice();
//        delete m_controller;
//        m_controller = nullptr;
    }
    SendMessage("~Controller");
}

void Controller::setIgnoreVersionCompare()
{
    m_service->setIgnoreVersionCompare();
}

void Controller::SetProperty(QByteArrayList &data, QByteArrayList &name, int size, QByteArray &version)
{
    m_service->SetProperty(data, name, size, version);
}

void Controller::ConnectDevice(const QBluetoothDeviceInfo &info, int timeout)
{
    if(m_controller)
    {
        auto state = m_controller->state();
        if (QLowEnergyController::ConnectingState == state
                || QLowEnergyController::ConnectedState == state)
        {
            SendMessage("igoring repeat connect");
            return;
        }
        SendMessage(QString::number(m_timeout_count) + "disconnectFromDevice");
        m_controller->disconnectFromDevice();
        SendMessage("delete m_controller");
        //delete m_controller;
        //m_controller = NULL;
        QThread::sleep(1);
    }

    m_device_info = info;
    m_controller = QLowEnergyController::createCentral(info);
    connect(m_controller, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_controller, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(m_controller, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(onStateChanged(QLowEnergyController::ControllerState)));
    connect(m_controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onError(QLowEnergyController::Error)));
    connect(m_controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(onServiceDiscovered(QBluetoothUuid)));
    connect(m_controller, SIGNAL(discoveryFinished()), this, SLOT(onDiscoveryFinished()));
    connect(m_controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(onConnectionUpdated(QLowEnergyConnectionParameters)));

    m_startOTATimer = new QTimer();
    connect(m_startOTATimer, &QTimer::timeout, this, &Controller::onStartOTATimeout);

    m_controller->connectToDevice();
    m_startOTATimer->start(timeout*1000);
    SendMessage("ConnectDevice timeout:"+QString::number(timeout));
}

void Controller::DisconnectDevice()
{
    deviceError();
}

QLowEnergyService * Controller::CreateService(QBluetoothUuid serviceUUID)
{
    if (m_controller)
    {
        return m_controller->createServiceObject(serviceUUID);
    }
    else
    {
        return NULL;
    }
}

void Controller::SendMessage(const QString &str)
{
    qInfo() << "Controller:" << m_device_info.address().toString()
            << str;
}

void Controller::onConnected()
{
    if (m_controller)
    {
        m_controller->discoverServices();

        SendMessage("device connected");
    }
}

void Controller::deviceError()
{
    emit upgradeResult(false, m_device_info.address().toString());
}

void Controller::onDisconnected()
{
//    SendMessage("device disconnected");
//    if (m_timeout_count --) {
//        SendMessage(QString("onReconnectDevice timeout_count: %1").arg(m_timeout_count));
//        ConnectDevice(m_device_info);
//    } else {
//        SendMessage("try to reconnect device failed...");
//        deviceError();
//    }
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

        qWarning() << "Controller:" << m_controller->remoteAddress().toString()
                   << str;
        if (m_startOTA)
        {
            //the error after ota
            DisconnectDevice();
        }
        else
        {
            //ota is not beginning
            emit startError();
        }
    }
}

void Controller::onServiceDiscovered(QBluetoothUuid serviceUUID)
{
    if (0x27f0 == serviceUUID.data1
            || 0x0af0 == serviceUUID.data1) {
        QLowEnergyService *service= CreateService(serviceUUID);
        emit serviceDiscovered(service, m_device_info.address().toString());
    }
}

void Controller::onStartError()
{
    m_startOTATimer->stop();
    qWarning() << "Controller:" << m_device_info.address().toString()
               << "start error";
    deviceError();
}

void Controller::onStartOTA()
{
    m_startOTATimer->stop();
    SendMessage("service begin to ota");
    m_startOTA = true;
}

void Controller::onStartOTATimeout()
{
    if (!m_startOTA)
    {
        qWarning() << "Controller:" << m_device_info.address().toString()
                   << "start ota timeout";
        deviceError();
    }
}

void Controller::onDiscoveryFinished()
{
    SendMessage("service discovery finished");
//    QList<QBluetoothUuid> uuids= m_controller->services();
//    for (const auto &uuid : qAsConst(uuids)) {
//        if (0x27f0 == uuid.data1
//                || 0x0af0 == uuid.data1) {
//            QLowEnergyService *service = CreateService(uuid);
//            emit serviceDiscovered(service, m_controller->remoteAddress().toString());
//        }
//    }
}

void Controller::onConnectionUpdated(const QLowEnergyConnectionParameters &parameters)
{
    Q_UNUSED(parameters);
    SendMessage("controller connect updated");
}

void Controller::onReconnectDevice()
{

}
