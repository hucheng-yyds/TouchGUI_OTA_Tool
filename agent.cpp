#include "agent.h"
#include <QDebug>

Agent::Agent(QObject *parent) : QObject(parent)
{
    m_agent = new QBluetoothDeviceDiscoveryAgent(this);

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
    }
}

void Agent::SendMessage(QString msg)
{
    qDebug() << "Agent" << msg;
    emit message(msg);
}

void Agent::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
//    if (-1 == info.name().indexOf("realme Watch")
//            || -1 == info.name().lastIndexOf("3295")) {
//        return ;
//    }
    if (m_address_list.filter(
                info.address().toString().right(5)).isEmpty()) {
        return ;
    }

    emit deviceDiscovered(info);
    QString tmp = "发现设备:";
    QString str = info.address().toString() + " - " + info.name();
    SendMessage(tmp + str);
}

void Agent::onError(QBluetoothDeviceDiscoveryAgent::Error err)
{
    QString str;

    str = QString("Agent Error(%1):").arg(err);
    str += m_agent->errorString();

    SendMessage(str);
}

void Agent::onFinished()
{
    SendMessage("Agent scan finished");
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
