#include "agent.h"
#include <QDebug>

Agent::Agent(QObject *parent) : QObject(parent)
{
    m_agent = new QBluetoothDeviceDiscoveryAgent(this);
    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &Agent::onStartAgentScan);
    connect(m_agent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(onDeviceDiscovered(QBluetoothDeviceInfo)));
    connect(m_agent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(onError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_agent, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(m_agent, SIGNAL(canceled()), this, SLOT(onCanceled()));

    connect(this, &Agent::startAgentScan, this, &Agent::onStartAgentScan);
    connect(this, &Agent::stopAgentScan, this, &Agent::onStopAgentScan);
}

Agent::~Agent()
{
    stopScan();
    delete m_agent;
    delete m_timer;
    m_agent = nullptr;
    m_timer = nullptr;
}

void Agent::initScanData(int msTimeout, const QStringList &address)
{
    m_address_list = address;
    m_agent->setLowEnergyDiscoveryTimeout(msTimeout);
}

void Agent::startScanDevice(uint32_t timeOut, const QStringList &address)
{
    if(m_agent)
    {
        m_address_list = address;
        m_agent->setLowEnergyDiscoveryTimeout(timeOut);
        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        SendMessage("startScan...");
        //m_timer->start(10 * 1000 + timeOut);
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

void Agent::onStartAgentScan()
{
    stopTimer();
    stopScan();
    try {
        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        SendMessage("scan started...");
    } catch (...) {
        SendMessage("start scan error exception...");
    }
}

void Agent::onStopAgentScan()
{
    stopScan();
}

void Agent::startTimer()
{
    if (!m_timer->isActive())
    {
        m_timer->start(20*1000);
        SendMessage("scan timer started...");
    }
}

void Agent::stopTimer()
{
    if (m_timer->isActive())
    {
        m_timer->stop();
        SendMessage("scan timer stopped...");
    }
}

void Agent::stopScan()
{
    if (m_agent->isActive())
    {
        m_agent->stop();
        SendMessage("scan stopped...");
    }
}

bool Agent::isActive()
{
    return
            m_timer->isActive() ||
            m_agent->isActive();
}

void Agent::setMatchStr(const QString &matchStr)
{
    m_match_str = matchStr;
}

void Agent::SendMessage(const QString &msg)
{
    qInfo() << "Agent" << msg;
}

void Agent::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!info.isValid())
    {
        return;
    }
    if (m_match_str.isEmpty()) {
        bool find = false;
        //m_address_mutex.lock();
        if (!m_address_list.filter(
                    info.address().toString()
                    ,Qt::CaseInsensitive).isEmpty()) {
            find = true;
        }
        //m_address_mutex.unlock();
        if (!find)
        {
            return;
        }

    } else {
        if (-1 == info.name().indexOf(m_match_str)) {
            return ;
        }
    }
    emit deviceDiscovered(info);
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
    m_not_find = true; 

    if (isFindCountEnough()
            && ((m_processingcount+m_successcount)<m_targetcount))
    {
        startTimer();
    }
    else
    {
        stopTimer();
    }
    qInfo() << "processing_count:" << m_processingcount
            << "success_count:" << m_successcount
            << "target_count:" << m_targetcount;
}

void Agent::onCanceled()
{
    SendMessage("scan canceled");
}

void Agent::increaseSuccessCount(const QString & succ_address)
{
    //m_address_mutex.lock();
    m_address_list.removeOne(succ_address);
    m_successcount++;
    //m_address_mutex.unlock();
}
