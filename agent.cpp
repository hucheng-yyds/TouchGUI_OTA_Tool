#include "agent.h"
#include <QDebug>

Agent::Agent(QObject *parent) : QObject(parent)
{
    m_agent = new QBluetoothDeviceDiscoveryAgent(this);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    });

    if(m_agent)
    {
        connect(m_agent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(onDeviceDiscovered(QBluetoothDeviceInfo)));
        connect(m_agent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(onError(QBluetoothDeviceDiscoveryAgent::Error)));
        connect(m_agent, SIGNAL(finished()), this, SLOT(onFinished()));
        connect(m_agent, SIGNAL(canceled()), this, SLOT(onCanceled()));
    }
}

void Agent::startScanDevice(uint32_t timeOut, const QStringList &address)
{
    if(m_agent)
    {
        m_address_list = address;
        m_agent->setLowEnergyDiscoveryTimeout(timeOut);
        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        SendMessage("scanning...");
        m_timer->start(10 * 1000 + timeOut);
//        int i = 0;
//        for (const auto &string : address) {
//            QBluetoothAddress mac(string);
//            if (mac.isNull()) {
//                continue ;
//            }
//            const QBluetoothDeviceInfo info(mac, "", 0);
//            emit deviceDiscovered(info);
//            if (++ i >= 7) {
//                break ;
//            }
//        }
    }
}

void Agent::stopScan()
{
    m_agent->stop();
    m_timer->stop();
}

bool Agent::isActive()
{
    return m_timer->isActive() || m_agent->isActive();
}

void Agent::setMatchStr(const QString &matchStr)
{
    m_match_str = matchStr;
}

void Agent::SendMessage(const QString &msg)
{
    qInfo() << "Agent" << msg;
    emit message(msg);
}

void Agent::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (m_match_str.isEmpty()) {
        if (m_address_list.filter(
                    info.address().toString()
                    ,Qt::CaseInsensitive).isEmpty()) {
            return ;
        }
    } else {
        if (-1 == info.name().indexOf(m_match_str)) {
            return ;
        }
    }
    emit deviceDiscovered(info);
    m_address_size ++;
    m_not_find = false;
    QString tmp = "发现设备:";
    QString str = info.address().toString() + " - " + info.name() + " rssi:" + QString::number(info.rssi());
    SendMessage(tmp + str);
}

void Agent::onError(QBluetoothDeviceDiscoveryAgent::Error err)
{
    QString str;

    str = QString("Error(%1):").arg(err);
    str += m_agent->errorString();

    qWarning() << "Agent" << str;
}

void Agent::onFinished()
{
    SendMessage("scan finished");
    if (m_not_find) {
        m_find_count ++;
    } else {
        m_find_count = 0;
    }
    if (m_successcount >= m_targetcount || m_find_count >= 6) {
        m_timer->stop();
        emit scanFinished();
        SendMessage("scan stop");
    }
    m_not_find = true;
//    const QList<QBluetoothDeviceInfo> foundDevices = m_agent->discoveredDevices();
//    for (const auto &nextDevice : foundDevices) {
//        for (const auto &str : qAsConst(m_address_list)) {
//            if (str == nextDevice.address().toString().right(5)) {
//                qDebug() << "find devices:" << nextDevice.name();
//                emit deviceDiscovered(nextDevice);
//            }
//        }
//    }
}

void Agent::onCanceled()
{
    SendMessage("Agent scan canceled");
}
