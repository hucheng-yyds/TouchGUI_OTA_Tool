#include "controller.h"

#define print  qInfo() << m_device.address().toString()// << QThread::currentThreadId()
QSemaphore Controller::g_Semaphore(1);
Controller::Controller(const QBluetoothDeviceInfo &info)
{
    qRegisterMetaType<Service::ResultState>("Service::ResultState");
    m_device = info;
    m_thread = new QThread;
    moveToThread(m_thread);
    connect(this, &Controller::connectToDevice, this, &Controller::onConnectToDevice);

    m_service = new Service;
    m_service->moveToThread(m_thread);
    connect(this, &Controller::serviceDiscovered, m_service, &Service::ConnectService);
    connect(m_service, &Service::upgradeResult, this, &Controller::DisconnectDevice);
    connect(m_service, &Service::startOTA, this, &Controller::onStartOTA);

    m_startOTATimer = new QTimer;
    m_startOTATimer->moveToThread(m_thread);
    m_startOTATimer->setSingleShot(true);
    connect(m_startOTATimer, &QTimer::timeout, this, &Controller::onStartOTATimeout);
}

Controller::~Controller()
{
    m_controller->deleteLater();
    m_service->deleteLater();
    m_startOTATimer->deleteLater();
    m_thread->quit();
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    print << "~Controller";
}

void Controller::ConnectDevice(int timeout)
{
    m_thread->start();
    emit connectToDevice(timeout);
}

void Controller::DisconnectDevice(Service::ResultState state)
{
    m_startOTATimer->stop();
    m_controller->disconnectFromDevice();
//    m_controller->deleteLater();
//    m_controller = nullptr;
//    m_thread->quit();
    emit upgradeResult(state, address());
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
//        isLocked = true;
//        Controller::g_Semaphore.acquire();
//        print << "g_Semaphore is" << Controller::g_Semaphore.available();
        QTimer::singleShot(1000, this, [this]{
            m_controller->discoverServices();
        });
    }
}

void Controller::onDisconnected()
{
    if (isLocked) {
        isLocked = false;
        Controller::g_Semaphore.release();
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
        DisconnectDevice(Service::ConnectionException);
    }
}
#if 0
#include "windows.h"
__int64 Filetime2Int64(const FILETIME* ftime)
{
    LARGE_INTEGER li;
    li.LowPart = ftime->dwLowDateTime;
    li.HighPart = ftime->dwHighDateTime;
    return li.QuadPart;
}
//两个时间相减运算
__int64 CompareFileTime(FILETIME preTime, FILETIME nowTime)
{
    return Filetime2Int64(&nowTime) - Filetime2Int64(&preTime);
}
int calCpuUsage()// 耗时操作
{
        HANDLE hEvent;
        bool res;
        static FILETIME preIdleTime;
        static FILETIME preKernelTime;
        static FILETIME preUserTime;

        FILETIME idleTime;
        FILETIME kernelTime;
        FILETIME userTime;

        res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
        if (!res)
            return -1;

        preIdleTime = idleTime;
        preKernelTime = kernelTime;
        preUserTime = userTime;

        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        WaitForSingleObject(hEvent, 500);	//等待500毫秒

        res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
        if (!res)
            return -1;

        int idle = CompareFileTime(preIdleTime, idleTime);
        int kernel = CompareFileTime(preKernelTime, kernelTime);
        int user = CompareFileTime(preUserTime, userTime);

        auto nCpuRate = (int)ceil(100.0 * (kernel + user - idle) / (kernel + user));
        return nCpuRate;
}
#endif
void Controller::onConnectToDevice(int timeout)
{
//    qInfo() << "calCpuUsage:" << calCpuUsage();
    m_connect_count --;
    m_startOTATimer->setInterval(timeout * 1000);
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
    m_startOTATimer->start();
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
    if (isLocked) {
        isLocked = false;
        Controller::g_Semaphore.release();
    }
}

void Controller::onStartOTATimeout()
{
    print << "start ota timeout";
    DisconnectDevice(Service::ConnectionException);
}
