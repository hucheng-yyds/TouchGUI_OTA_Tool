#include "agent.h"
#include "setup.h"
#include <QDebug>

Agent::Agent(QObject *parent) : QObject(parent)
{
    m_agent = new QBluetoothDeviceDiscoveryAgent(this);
    m_agent->setLowEnergyDiscoveryTimeout(setup->m_scanTimeout * 1000);
//    m_agent->setLowEnergyDiscoveryTimeout(0);
    m_timer = new QTimer(this);
    m_timer->setInterval(setup->m_scanTimeout * 1000);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, m_agent, &QBluetoothDeviceDiscoveryAgent::stop);
    connect(m_agent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(onDeviceDiscovered(QBluetoothDeviceInfo)));
    connect(m_agent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(onError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_agent, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(m_agent, SIGNAL(canceled()), this, SLOT(onCanceled()));
}

bool Agent::isActive()
{
    return m_timer->isActive() || m_agent->isActive();
}

void Agent::setMatchStr(const QString &matchStr)
{
    m_match_str = matchStr;
}
//没有找到一个设备算一次消耗，找到设备消耗重置为0
bool Agent::isFindCountEnough() const
{
    return m_find_count < 6;
}

void Agent::startScan()
{
    m_cancel = false;
    try {
        qDebug("scan started...");
#if 1
        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
#else
    //直连问题多！controller析构无法正常调用
    for (const auto &string : qAsConst(setup->m_address_list)) {
        QBluetoothAddress mac(string);
        if (mac.isNull()) {
            continue ;
        }
        const QBluetoothDeviceInfo info(mac, "", 0);
        emit deviceDiscovered(info);
    }
    emit scanFinished(false);
#endif
//        startTimer();
    } catch (...) {
        qDebug("start scan error exception...");
    }
}

void Agent::stopScan()
{
    if (m_agent->isActive())
    {
        m_agent->stop();
        qDebug("scan stopped...");
    }
    stopTimer();
}

void Agent::cancelScan()
{
    m_cancel = true;
    stopScan();
}

void Agent::startTimer()
{
    if (!m_timer->isActive())
    {
        m_timer->start();
        qDebug("scan timer started...");
    }
}

void Agent::stopTimer()
{
    if (m_timer->isActive())
    {
        m_timer->stop();
        qDebug("scan timer stopped...");
    }
}

void Agent::resetData()
{
    m_find_count = 0;
    m_not_find = true;
}

void Agent::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (m_match_str.isEmpty()) {
        if (!setup->m_address_list.contains(info.address().toString(), Qt::CaseInsensitive)) {
            return ;
        }
    } else {
        if (!info.name().contains(m_match_str, Qt::CaseInsensitive)) {
            return ;
        }
    }
    m_not_find = false;
    QString tmp = QString::fromLocal8Bit("发现设备:");
    QString str = info.address().toString() + " - " + info.name() + " rssi:" + QString::number(info.rssi());
    qInfo() << tmp << str;
    emit deviceDiscovered(info);
}

void Agent::onError(QBluetoothDeviceDiscoveryAgent::Error err)
{
    QString str;

    str = QString("Error(%1):").arg(err);
    str += m_agent->errorString();

    qWarning() << str;
}

//触发finished信号 会导致程序crash 2022-7-20 未解决
void Agent::onFinished()
{
    qInfo() << "scan finished";
    emit scanFinished(true);
}

void Agent::onCanceled()
{
    if (m_cancel) {
        return ;
    }
    if (m_not_find) {
        m_find_count ++;
    } else {
        m_find_count = 0;
    }
    m_not_find = true;

//    const QList<QBluetoothDeviceInfo> foundDevices = m_agent->discoveredDevices();
//    for (const auto &nextDevice : foundDevices) {
//        if (setup->m_address_list.contains(nextDevice.address().toString(), Qt::CaseInsensitive)) {
//            emit deviceDiscovered(nextDevice);
//        }
//    }
    qInfo("scan canceled");
    emit scanFinished(false);
}
