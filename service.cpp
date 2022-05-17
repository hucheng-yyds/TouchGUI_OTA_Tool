#include "service.h"
#include <QTimer>
#include <QEventLoop>

Service::Service(QObject *parent) : QObject(parent)
{
    m_service = NULL;
}

void Service::ConnectService(QLowEnergyService * service)
{
    m_service = service;
    if(m_service)
    {
       /* if(m_service->state() == QLowEnergyService::ServiceDiscovered)
        {
            onStateChanged(QLowEnergyService::ServiceDiscovered);
        }
        else */if (m_service->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(onStateChanged(QLowEnergyService::ServiceState)));
            connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicChanged(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicRead(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicWritten(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(descriptorRead(QLowEnergyDescriptor, QByteArray)), this, SLOT(onDescriptorRead(QLowEnergyDescriptor, QByteArray)));
            connect(m_service, SIGNAL(descriptorWritten(QLowEnergyDescriptor, QByteArray)), this, SLOT(onDescriptorWritten(QLowEnergyDescriptor, QByteArray)));
            connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(onError(QLowEnergyService::ServiceError)));
            qDebug() << "discoverDetails:" << m_service->state();
            m_service->discoverDetails();
        }
    }
}

void Service::SendMessage(QString msg)
{
    qDebug() << "Service:" << msg;
    emit message(msg);
}

void Service::OpenNotify(QLowEnergyCharacteristic ch, bool flag)
{
    if(m_service)
    {
        if(ch.isValid())
        {
            if(ch.properties() & QLowEnergyCharacteristic::Notify)
            {
                QLowEnergyDescriptor d = ch.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                if(d.isValid())
                {
                    if(true == flag)
                    {
                        m_service->writeDescriptor(d, QByteArray::fromHex("0100"));
                    }
                    else
                    {
                        m_service->writeDescriptor(d, QByteArray::fromHex("0000"));
                    }
                }
            }
        }
    }
}

void Service::ReadCharacteristic(QLowEnergyCharacteristic ch)
{
    if(m_service)
    {
        if(ch.isValid())
        {
            if(ch.properties() & QLowEnergyCharacteristic::Read)
            {
                m_service->readCharacteristic(ch);
            }
        }
    }
}

void Service::WriteCharacteristic(QLowEnergyCharacteristic ch, const QByteArray &arr)
{
    qDebug() << __FUNCTION__ << arr.left(10).toHex('|');
    if(m_service)
    {
        if(ch.isValid())
        {
            if(ch.properties() & QLowEnergyCharacteristic::Write)
            {
                m_service->writeCharacteristic(ch, arr, QLowEnergyService::WriteWithResponse);
            }
            else if(ch.properties() & QLowEnergyCharacteristic::WriteNoResponse)
            {
                m_service->writeCharacteristic(ch, arr, QLowEnergyService::WriteWithoutResponse);
            }
            else if(ch.properties() & QLowEnergyCharacteristic::WriteSigned)
            {
                m_service->writeCharacteristic(ch, arr, QLowEnergyService::WriteSigned);
            }
        }

    }
}

void Service::SendCmdKeyData(const uchar cmd, const uchar key, QByteArray &byte)
{
    byte.prepend(key);
    byte.prepend(cmd);
    WriteCharacteristic(m_characteristics_list[0], byte);
}

void Service::StartSendData()
{
    QByteArray byte;
    byte.append(getFileType(m_file_index));
    byte.append(m_file_data_list[m_file_index].size());
    byte.append(m_file_data_list[m_file_index].size() >> 8);
    byte.append(m_file_data_list[m_file_index].size() >> 16);
    byte.append(m_file_data_list[m_file_index].size() >> 24);
    byte.append((char)0x00);
    byte.append(m_file_name_list[m_file_index]);
    SendCmdKeyData(CMD_HEAD_OTA, CMD_SEND_START, byte);
}

void Service::StopSendData()
{
    QByteArray byte;
    byte.append(m_check_sum);
    byte.append(m_check_sum >> 8);
    byte.append(m_check_sum >> 16);
    byte.append(m_check_sum >> 24);
    byte.append(QCryptographicHash::hash(m_file_data_list[m_file_index],
                                         QCryptographicHash::Md5).toHex());
    SendCmdKeyData(CMD_HEAD_OTA, CMD_SEND_END, byte);
}

uchar Service::getFileType(int index)
{
    uchar c = 0;
    switch (index) {
    case 0:
        c = 0x23;
        break;
    case 1:
        c = 0x11;
        break;
    case 2:
    case 3:
    case 4:
        c = 0xff;
        break;
    default:
        break;
    }
    return c;
}

uint32_t Service::CheckSum(uint8_t *pBuffer, uint8_t len)
{
    uint32_t   csum = 0;
    uint8_t    i = 0;

    if (pBuffer == NULL)
        return 0;

    while(i < len)
    {
        csum += pBuffer[i++];
    }
    return csum;
}

void Service::onStateChanged(QLowEnergyService::ServiceState newState)
{
    if(m_service)
    {
        switch(newState)
        {
        case QLowEnergyService::DiscoveringServices:
        {
            SendMessage("Discovering services...");
        }
            break;

        case QLowEnergyService::DiscoveryRequired:
        {
            SendMessage("require discover services...");
        }
            break;

        case QLowEnergyService::ServiceDiscovered:
        {
            SendMessage("Discovered services");
            QList<QLowEnergyCharacteristic> characteristics = m_service->characteristics();
            qDebug() << characteristics.size();
//            qDeleteAll(m_characteristics_list);
            m_characteristics_list.clear();
            m_characteristics_list = characteristics;

            foreach(QLowEnergyCharacteristic ch, characteristics)
            {
                qDebug() << ch.uuid();
                if (ch.uuid().data1 == 0x27f7
                        || ch.uuid().data1 == 0x0af7) {
                    OpenNotify(ch, true);
                    emit discoveryCharacteristic(ch);
                }
            }
            QByteArray byte(2, 0);
            byte[0] = CMD_HEAD_PARAM;
            byte[1] = KEY_GET_INFO;
            WriteCharacteristic(m_characteristics_list[0], byte);
            byte[0] = CMD_HEAD_PARAM;
            byte[1] = KEY_GET_MTU;
            WriteCharacteristic(m_characteristics_list[0], byte);
        }
            break;

        default:
            break;
        }
    }
}

void Service::onCharacteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    QString ch = info.uuid().toString() + " - Characteristic Changed:" + value.toHex('|');
    SendMessage(ch);
    QByteArray byte;
    if ((uchar)value[0] == CMD_HEAD_OTA) {
        switch (value[1]) {
        case CMD_SEND_START:
            byte.append((char)m_device_prn);
            SendCmdKeyData(CMD_HEAD_OTA, CMD_SET_PRN, byte);
            break;
        case CMD_SEND_BODY:
            qDebug() << "m_file_offset:" << m_file_offset
                     << "m_file_data_list size:" << m_file_data_list[m_file_index].size();
            m_timeout_num = 3;
            m_cur_sum = (value[3] & 0xFF)
                    | ((value[4] & 0xFF) << 8)
                    | ((value[5] & 0xFF) << 16)
                    | ((value[6] & 0xFF) << 24);
            qDebug() << "devices sum:" << m_cur_sum
                     << "service sum:" << m_check_sum;
            if (m_check_sum == m_cur_sum) {
                m_package_num = 0;
            } else {
                qDebug() << "check_sum error";
            }
            break;
        case CMD_SEND_END:
            if (0 == value[2]) {
                m_package_num = 0;
                m_file_offset = 0;
                m_check_sum = 0;
                m_file_index ++;
                if (m_file_index >= m_file_data_list.size()) {
                    m_file_index = 0;
                    break;
                }
            }
            StartSendData();
            break;
        case CMD_SET_PRN:
            if (value[2] != 0) {
                m_device_prn = value[2];
            }
            for (int i = 0; i < m_file_data_list[m_file_index].size(); i += m_device_mtu) {
                m_file_offset = i;
                m_package_num ++;
                if ((i + m_device_mtu) >= m_file_data_list[m_file_index].size()) {
                    m_file_offset = i + m_device_mtu;
                }
                byte = m_file_data_list[m_file_index].mid(i, m_device_mtu);
//                qDebug() << "byte.size():" << byte.size();
                m_check_sum += CheckSum((uchar*)byte.data(), byte.size());
                byte.prepend((char)0x00);
send_data:
                QEventLoop eventloop;
                SendCmdKeyData(CMD_HEAD_OTA, CMD_SEND_BODY, byte);
//                QTimer::singleShot(5000, &eventloop, &QEventLoop::quit);
//                connect(m_service, &QLowEnergyService::characteristicWritten, &eventloop, &QEventLoop::quit);
//                eventloop.exec();
                if (m_package_num >= m_device_prn) {
                    qDebug() << "fill package" << m_package_num;
                    QTimer::singleShot(5000, &eventloop, &QEventLoop::quit);
                    connect(m_service, &QLowEnergyService::characteristicChanged, &eventloop, &QEventLoop::quit);
                    eventloop.exec();
                    if (m_package_num) {
                        m_timeout_num --;
                        qDebug() << "recv d1 02 time out" << m_timeout_num;
                        if (m_timeout_num < 0) {
                            break ;
                        } else {
                            goto send_data;
                        }
                    }
                }
                if (m_file_offset >= m_file_data_list[m_file_index].size()) {
                    StopSendData();
//                    if (m_package_num) {
//                        m_timeout_num --;
//                        qDebug() << "recv d1 03 time out" << m_timeout_num;
//                        if (m_timeout_num < 0) {
//                            break ;
//                        } else {
//                            goto send_data;
//                        }
//                    }
                }
            }
            break;
        default:
            break;
        }
    } else if (value[0] == CMD_HEAD_PARAM
               && value[1] == KEY_GET_INFO) {//校验设备信息：版本号、模式、电量

    } else if (value[0] == CMD_HEAD_PARAM
               && (uchar)value[1] == KEY_GET_MTU) {//校验MTU包大小
        m_device_mtu &= (value[2] << 8) + value[3];
        byte.append(m_total_file_size);
        byte.append(m_total_file_size >> 8);
        byte.append(m_total_file_size >> 16);
        byte.append(m_total_file_size >> 24);
        byte.append(QByteArray(4, 0));
        SendCmdKeyData(CMD_HEAD_OTA, CMD_SET_PROGRESS, byte);
        StartSendData();
    }
}

void Service::onCharacteristicRead(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    QString ch = info.uuid().toString() + " - Characteristic read:" + QString(value);
    SendMessage(ch);
}

void Service::onCharacteristicWritten(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    QString ch = info.uuid().toString() + " - Characteristic written:" + value.left(10).toHex('|');
//    SendMessage(ch);
}

void Service::onDescriptorRead(const QLowEnergyDescriptor &info, const QByteArray &value)
{
    QString ch = info.uuid().toString() + " - descriptor read:" + QString(value);
    SendMessage(ch);
}

void Service::onDescriptorWritten(const QLowEnergyDescriptor &info, const QByteArray &value)
{
    QString ch = info.uuid().toString() + " - descriptor written:" + QString(value);
    SendMessage(ch);
}

void Service::onError(QLowEnergyService::ServiceError error)
{

    const QString ServiceError[] {
        "NoError",
        "OperationError",
        "CharacteristicWriteError",
        "DescriptorWriteError",
        "UnknownError",
        "CharacteristicReadError",
        "DescriptorReadError"
    };

    if(error < ServiceError->size())
    {
        QString str;

        str = QString("service Error(%1):").arg(error);
        str += ServiceError[error];

        SendMessage(str);
    }
}
