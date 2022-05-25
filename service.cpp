#include "service.h"
#include <QTimer>
#include <QEventLoop>

Service::Service(QObject *parent) : QObject(parent)
{
    m_service = NULL;
}

void Service::SetProperty(QByteArrayList &data, QByteArrayList &name, int size)
{
    m_file_data_list = data;
    m_file_name_list = name;
    m_total_file_size = size;
}

void Service::ConnectService(QLowEnergyService * service, const QString &address)
{
    m_service = service;
    m_address = address;
    if(m_service)
    {
        if(m_service->state() == QLowEnergyService::ServiceDiscovered)
        {
            onStateChanged(QLowEnergyService::ServiceDiscovered);
        }
        else if (m_service->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(onStateChanged(QLowEnergyService::ServiceState)));
            connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicChanged(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicRead(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicWritten(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(descriptorRead(QLowEnergyDescriptor, QByteArray)), this, SLOT(onDescriptorRead(QLowEnergyDescriptor, QByteArray)));
            connect(m_service, SIGNAL(descriptorWritten(QLowEnergyDescriptor, QByteArray)), this, SLOT(onDescriptorWritten(QLowEnergyDescriptor, QByteArray)));
            connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(onError(QLowEnergyService::ServiceError)));
            SendMessage("discoverDetails:" + QString::number(m_service->state()));
            m_service->discoverDetails();
//            QTimer::singleShot(500, this, [this]() {
//                m_service->discoverDetails();
//            });
        }
    }
}

void Service::SendMessage(QString msg)
{
    qDebug() << "Service:" << m_address << msg;
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
//    qDebug() << __FUNCTION__ << arr.left(10).toHex('|');
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

            foreach(QLowEnergyCharacteristic ch, characteristics)
            {
                qDebug() << ch.uuid();
                if (ch.uuid().data1 == 0x27f6
                        || ch.uuid().data1 == 0x0af6) {
                    m_characteristics = ch;
                }
                if (ch.uuid().data1 == 0x27f7
                        || ch.uuid().data1 == 0x0af7) {
                    OpenNotify(ch, true);
                    emit discoveryCharacteristic(ch);
                }
            }
            QByteArray byte(2, 0);
            byte[0] = CMD_HEAD_PARAM;
            byte[1] = KEY_GET_MTU;
            WriteCharacteristic(m_characteristics, byte);
            byte[0] = CMD_HEAD_PARAM;
            byte[1] = KEY_GET_INFO;
            WriteCharacteristic(m_characteristics, byte);
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
            m_cur_sum = (value[3] & 0xFF)//获取设备当前checksum
                    | ((value[4] & 0xFF) << 8)
                    | ((value[5] & 0xFF) << 16)
                    | ((value[6] & 0xFF) << 24);
            m_cur_offset = (value[7] & 0xFF)//获取设备当前offset
                    | ((value[8] & 0xFF) << 8)
                    | ((value[9] & 0xFF) << 16)
                    | ((value[10] & 0xFF) << 24);
            qDebug() << m_address
                     << "m_file_name:" << m_file_name_list[m_file_index]
                     << "m_file_size:" << m_file_data_list[m_file_index].size()
                     << "m_cur_sum:" << m_cur_sum
                     << "m_check_sum:" << m_check_sum
                     << "m_cur_offset:" << m_cur_offset
                     << "m_file_offset:" << m_file_offset;
            if (m_check_sum == m_cur_sum) {//校验客户端与设备的checksum
                m_package_num = 0;
            } else {
                qDebug() << "check_sum error";
            }
            break;
        case CMD_SEND_END:
            if (0 == value[2]) {//文件发送完成
                m_last_pack = false;
                m_package_num = 0;
                m_file_offset = 0;
                m_check_sum = 0;
                m_file_index ++;
                if (m_file_index >= m_file_data_list.size()) {
                    emit upgradeResult(true, m_address);
                    break;
                }
                StartSendData();
            }
            break;
        case CMD_SET_PRN:
            if (0 != value[2]) {//成功获取PRN
                m_device_prn = value[2];
            }
            for (int i = 0; i < m_file_data_list[m_file_index].size(); i += m_device_mtu) {
                //开始发送数据
                m_file_offset = i + m_device_mtu;
                m_package_num ++;
                if (m_file_offset >= m_file_data_list[m_file_index].size()) {
                    //处理最后一包数据
                    m_last_pack = true;
                }
                byte = m_file_data_list[m_file_index].mid(i, m_device_mtu);
                m_check_sum += CheckSum((uchar*)byte.data(), byte.size());
                byte.prepend((char)0x00);
                SendCmdKeyData(CMD_HEAD_OTA, CMD_SEND_BODY, byte);
                if (m_package_num >= m_device_prn) {
                    WaitReplyData(5);
                    if (m_package_num) {
                        qDebug() << i << m_address << "recv d1 02 time out";
                        emit reconnectDevice();
                        break ;
                    }
                }
                if (m_last_pack) {
                    StopSendData();
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

void Service::SendCmdKeyData(const uchar cmd, const uchar key, QByteArray &byte)
{
    byte.prepend(key);
    byte.prepend(cmd);
    WriteCharacteristic(m_characteristics, byte);
}

void Service::StartSendData()
{
    m_last_pack = false;
    m_package_num = 0;
    m_file_offset = 0;
    m_check_sum = 0;

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

void Service::SetOffsetData()
{
    QByteArray byte;
    byte.append(m_cur_offset);
    byte.append(m_cur_offset >> 8);
    byte.append(m_cur_offset >> 16);
    byte.append(m_cur_offset >> 24);
    byte.append(m_cur_sum);
    byte.append(m_cur_sum >> 8);
    byte.append(m_cur_sum >> 16);
    byte.append(m_cur_sum >> 24);
    SendCmdKeyData(CMD_HEAD_OTA, CMD_SET_OFFSET, byte);
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

void Service::WaitReplyData(int secTimeout)
{
    QEventLoop eventloop;
    QTimer::singleShot(secTimeout*1000, &eventloop, &QEventLoop::quit);
    connect(m_service, &QLowEnergyService::characteristicChanged, &eventloop, &QEventLoop::quit);
    eventloop.exec();
}
