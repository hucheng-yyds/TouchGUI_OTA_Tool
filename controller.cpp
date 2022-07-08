#include "controller.h"
#include <QTime>
#include <QTimer>

Controller::Controller(QObject *parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_service = new Service;
    moveToThread(m_thread);
    m_service->moveToThread(m_thread);
    connect(this, &Controller::serviceDiscovered, m_service, &Service::ConnectService);
    connect(m_service, &Service::disconnectDevice, this, &Controller::DisconnectDevice);
    connect(m_service, &Service::upgradeResult, this, &Controller::upgradeResult);
    m_thread->start();
}

Controller::~Controller()
{
    m_thread->quit();
    SendMessage("~Controller");
    m_thread->wait();
    //delete m_thread;
    delete m_service;
    m_service = nullptr;
    if(m_controller)
    {
        m_controller->disconnectFromDevice();
        delete m_controller;
        m_controller = NULL;
    }
}

void Controller::SetProperty(QByteArrayList &data, QByteArrayList &name, int size, QByteArray &version)
{
    m_service->SetProperty(data, name, size, version);
}

void Controller::ConnectDevice(const QBluetoothDeviceInfo &info)
{
    if(m_controller)
    {
        SendMessage(QString::number(m_timeout_count) + "disconnectFromDevice");
        m_controller->disconnectFromDevice();
        SendMessage("delete m_controller");
        delete m_controller;
        m_controller = NULL;
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

    m_controller->connectToDevice();
    SendMessage("ConnectDevice");

    if (!WaitServiceStartOTAReply(30))
    {
        SendMessage("service start timout");
        deviceError();
    }
    else
    {
        SendMessage("service begin to ota");
    }
}

void Controller::DisconnectDevice()
{
    if(m_controller && QLowEnergyController::ConnectedState == m_controller->state())
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

void Controller::SendMessage(const QString &str)
{
    if (m_controller) {
        qDebug() << "Controller:" << m_controller->remoteAddress().toString()
                 //<< QTime::currentTime().toString("hh:mm:ss:zzz")
                 << QThread::currentThreadId() << str;
        emit message(str);
    }
}

void Controller::onConnected()
{
    if(m_controller)
    {
        m_controller->discoverServices();

        SendMessage("device connected");
    }
}

void Controller::deviceError()
{
    emit upgradeResult(false, m_controller->remoteAddress().toString());
}

void Controller::onDisconnected()
{
    SendMessage("device disconnected");
    if (m_timeout_count --) {
        SendMessage("onReconnectDevice");
        ConnectDevice(m_device_info);
    } else {
        SendMessage("try to reconnect device failed...");
        deviceError();
    }
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
        DisconnectDevice();
    }
}

void Controller::onServiceDiscovered(QBluetoothUuid serviceUUID)
{
    if (0x27f0 == serviceUUID.data1
            || 0x0af0 == serviceUUID.data1) {
        QLowEnergyService *service= CreateService(serviceUUID);
        if(m_controller) {
            emit serviceDiscovered(service, m_controller->remoteAddress().toString());
        }
    }
}

bool Controller::WaitServiceStartOTAReply(int secTimeout)
{
    bool reply_succ = true;

    QEventLoop eventloop;
    connect(m_service, &Service::startOTA, &eventloop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, [&eventloop,&reply_succ]{reply_succ=false; eventloop.quit();});
    timer.start(secTimeout * 1000);

    eventloop.exec();
    timer.stop();
    return reply_succ;
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
