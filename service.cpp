#include "service.h"
#include "setup.h"
#include <QTime>
#include <QTimer>
#include <QCryptographicHash>

#define print   qInfo() << m_address
#define printd  qDebug() << m_address
#define printw  qWarning() << m_address
#define printc  qCritical() << m_address

Service::Service(QObject *parent) : QObject(parent)
{
    QString tmp = "S.22B.51.66";
    m_oy22b_tpversion = tmp.toUtf8();
    m_oy22b_tpversion.resize(12);
}

Service::~Service()
{
    print << "~Service";
}

void Service::ConnectService(QLowEnergyService * service, const QString &address)
{
    m_service = service;
    m_address = address;
    if(m_service)
    {
        auto state = m_service->state();
        print << state;
        if(state == QLowEnergyService::ServiceDiscovered)
        {
            onStateChanged(QLowEnergyService::ServiceDiscovered);
        }
        else if (state == QLowEnergyService::DiscoveryRequired)
        {
            connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(onStateChanged(QLowEnergyService::ServiceState)));
            connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicChanged(QLowEnergyCharacteristic, QByteArray)));
//            connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicRead(QLowEnergyCharacteristic, QByteArray)));
//            connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic, QByteArray)), this, SLOT(onCharacteristicWritten(QLowEnergyCharacteristic, QByteArray)));
//            connect(m_service, SIGNAL(descriptorRead(QLowEnergyDescriptor, QByteArray)), this, SLOT(onDescriptorRead(QLowEnergyDescriptor, QByteArray)));
//            connect(m_service, SIGNAL(descriptorWritten(QLowEnergyDescriptor, QByteArray)), this, SLOT(onDescriptorWritten(QLowEnergyDescriptor, QByteArray)));
            connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(onError(QLowEnergyService::ServiceError)));
//            QThread::msleep(200);
            m_service->discoverDetails();
            print << "discoverDetails";
        }
    }
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
    printd << "Write Characteristic: " << arr.left(10).toHex('|');
    if(m_service)
    {
        if(ch.isValid())
        {
            /*if(ch.properties() & QLowEnergyCharacteristic::Write)
            {
                m_service->writeCharacteristic(ch, arr, QLowEnergyService::WriteWithResponse);
            }
            else */if(ch.properties() & QLowEnergyCharacteristic::WriteNoResponse)
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
        print << "state:" << newState;
        if (m_ota_finished && !setup->m_ota_poweroff)
        {
//            print << "OTA is already finished...";
//            emit upgradeResult(true, m_address);
            return;
        }

        switch(newState)
        {
        case QLowEnergyService::ServiceDiscovered:
        {
            QList<QLowEnergyCharacteristic> characteristics = m_service->characteristics();

            foreach(QLowEnergyCharacteristic ch, characteristics)
            {
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
            SendCmdKeyData(CMD_HEAD_SETUP, SETUP_AUTH);
            SendCmdKeyData(CMD_HEAD_PARAM, PARAM_GET_INFO);
        }
            break;
        default:
            break;
        }
    }
}

void Service::onCharacteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    Q_UNUSED(info);
    printd << " - Characteristic Changed:" << value.toHex('|');
    if ((uchar)value[0] == CMD_HEAD_OTA) {
        switch (value[1]) {
        case OTA_SEND_START:
            if (0 == value[2]) {
                SendCmdKeyData(CMD_HEAD_OTA, OTA_SET_PRN);
            } else if (CODE_SKIP_FILE == value[2]) {
                if ((m_file_index + 1) < setup->m_file_data_list.size()) {
                    m_file_index ++;
                    print << "CODE_SKIP_FILE: " + setup->m_file_name_list[m_file_index];
                    SendCmdKeyData(CMD_HEAD_OTA, OTA_SEND_START);
                } else {
                    m_ota_finished = true;
                    emit upgradeResult(true, m_address);
                }
            } else {
                printw << "fail: OTA_SEND_START";
                emit disconnectDevice();
            }
            break;
        case OTA_SEND_BODY:
            m_cur_sum = (value[3] & 0xFF)//��ȡ�豸��ǰchecksum
                    | ((value[4] & 0xFF) << 8)
                    | ((value[5] & 0xFF) << 16)
                    | ((value[6] & 0xFF) << 24);
            m_cur_offset = (value[7] & 0xFF)//��ȡ�豸��ǰoffset
                    | ((value[8] & 0xFF) << 8)
                    | ((value[9] & 0xFF) << 16)
                    | ((value[10] & 0xFF) << 24);
            qDebug() << m_address
                     << "m_file_index:" << m_file_index
                     << "m_file_name:" << setup->m_file_name_list[m_file_index]
                     << "m_file_size:" << setup->m_file_data_list[m_file_index].size()
                     << "m_cur_sum:" << m_cur_sum
                     << "m_check_sum:" << m_check_sum
                     << "m_cur_offset:" << m_cur_offset
                     << "m_file_offset:" << m_file_offset;
            if (CODE_SKIP_HEAD == value[2]) {
                m_set_offset = true;
                print << "CODE_SKIP_HEAD";
                SendCmdKeyData(CMD_HEAD_OTA, OTA_SET_OFFSET);
            }
            if (m_check_sum == m_cur_sum) {//У��ͻ������豸��checksum
                m_package_num = 0;
                emit recvOTABodyReply();
            } else {
                printc << "check sum fail";
            }
            break;
        case OTA_SEND_END:
            if (0 == value[2]) {//�ļ��������
                printd << "file_name:" << setup->m_file_name_list[m_file_index]
                       << "done";
                StopSendData();
                if ((m_file_index + 1) >= setup->m_file_data_list.size()) {
                    m_ota_finished = true;

                    if (setup->m_ota_poweroff)
                    {
                        SendCmdKeyData(CMD_HEAD_SYSTEM, SYSTEM_POWER_OFF);
                    }
                    else
                    {
                        SendCmdKeyData(CMD_HEAD_SYSTEM, SYSTEM_REBOOT);
                    }
                    print << "ota successfully...";
                    emit upgradeResult(true, m_address);
                } else {
                    m_file_index ++;
                    SendCmdKeyData(CMD_HEAD_OTA, OTA_SEND_START);
                }
            } else {
                printw << "fail: OTA_SEND_END";
                emit disconnectDevice();
            }
            break;
        case OTA_SET_OFFSET:
            break;
        case OTA_SET_PRN:
            if (0 != value[2]) {//�ɹ���ȡPRN
                m_device_prn = value[2];
                StartSendData();
            }
            break;
        case OTA_SET_PROGRESS:
            emit startOTA(m_address);
            SendCmdKeyData(CMD_HEAD_OTA, OTA_SEND_START);
            break;
        default:
            break;
        }
    } else if (value[0] == CMD_HEAD_PARAM) {
        switch ((uchar)value[1]) {
        case PARAM_GET_INFO:
            if (value[7] >= 30 && value[10] == 1) {
                SendCmdKeyData(CMD_HEAD_PARAM, PARAM_GET_VER);
            } else {
                printw << "battery:" << int(value[7]) << "has_detail_version:" << int(value[10]);
                emit upgradeResult(false, m_address);
            }
            break;
        case PARAM_GET_VER:
            if (!setup->m_ignore_version_compare
                    && !setup->m_version.isEmpty())
            {
                if (CompareVersion(setup->m_version, value.mid(8, 12)) <= 0
                        && (CompareVersion(m_oy22b_tpversion, value.mid(8, 12)) != 0))
                {
                    printw << "target version:" << setup->m_version
                           << "device version:" << value.mid(8, 12);
                    emit upgradeResult(true, m_address);
                    return;
                }
            }
            SendCmdKeyData(CMD_HEAD_OTA, OTA_SET_PROGRESS);
            break;
        case PARAM_GET_MTU:
            m_device_mtu &= (value[2] << 8) + value[3];
            SendCmdKeyData(CMD_HEAD_PARAM, PARAM_GET_VER);
            break;
        default:
            break;
        }
    }
}

//void Service::onCharacteristicRead(const QLowEnergyCharacteristic &info, const QByteArray &value)
//{
//    QString ch = info.uuid().toString() + " - Characteristic read:" + QString(value);
//}

//void Service::onCharacteristicWritten(const QLowEnergyCharacteristic &info, const QByteArray &value)
//{
//    QString ch = info.uuid().toString() + " - Characteristic written:" + value.left(10).toHex('|');
//}

//void Service::onDescriptorRead(const QLowEnergyDescriptor &info, const QByteArray &value)
//{
//    QString ch = info.uuid().toString() + " - descriptor read:" + QString(value);
//}

//void Service::onDescriptorWritten(const QLowEnergyDescriptor &info, const QByteArray &value)
//{
//    QString ch = info.uuid().toString() + " - descriptor written:" + QString(value);
//}

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

        printw << str;
        if (m_ota_finished)
        {
            //OTA�ɹ���Ĺػ�ָ����ܻᴥ�������쳣
            return;
        }
        emit disconnectDevice();
    }
}

void Service::SendCmdKeyData(const uchar cmd, const uchar key)
{
    QByteArray byte;
    byte.append(cmd);
    byte.append(key);
    if (CMD_HEAD_OTA == cmd) {
        switch (key) {
        case OTA_SEND_START:
            byte.append(getFileType(m_file_index));
            byte.append(setup->m_file_data_list[m_file_index].size());
            byte.append(setup->m_file_data_list[m_file_index].size() >> 8);
            byte.append(setup->m_file_data_list[m_file_index].size() >> 16);
            byte.append(setup->m_file_data_list[m_file_index].size() >> 24);
            byte.append((char)0x00);
            byte.append(setup->m_file_name_list[m_file_index]);
            break;
        case OTA_SEND_END:
            byte.append(m_check_sum);
            byte.append(m_check_sum >> 8);
            byte.append(m_check_sum >> 16);
            byte.append(m_check_sum >> 24);
            byte.append(QCryptographicHash::hash(setup->m_file_data_list[m_file_index],
                                                 QCryptographicHash::Md5).toHex());
            break;
        case OTA_SET_OFFSET:
            byte.append(m_cur_offset);
            byte.append(m_cur_offset >> 8);
            byte.append(m_cur_offset >> 16);
            byte.append(m_cur_offset >> 24);
            byte.append(m_cur_sum);
            byte.append(m_cur_sum >> 8);
            byte.append(m_cur_sum >> 16);
            byte.append(m_cur_sum >> 24);
            break;
        case OTA_SET_PRN:
            byte.append((char)m_device_prn);
            break;
        case OTA_GET_OFFSET:
            break;
        case OTA_SET_PROGRESS:
            byte.append(setup->m_total_file_size);
            byte.append(setup->m_total_file_size >> 8);
            byte.append(setup->m_total_file_size >> 16);
            byte.append(setup->m_total_file_size >> 24);
            byte.append(QByteArray(4, 0));
            break;
        default:
            break;
        }
    } else if (CMD_HEAD_SETUP == cmd) {
        byte.append(0x02);
        byte.append((char)0x00);
    }
    WriteCharacteristic(m_characteristics, byte);
}

void Service::StartSendData()
{
    StopSendData();
    for (int i = 0; i < setup->m_file_data_list[m_file_index].size(); i += m_device_mtu) {
        m_file_offset = i + m_device_mtu;
        m_package_num ++;
        if (m_file_offset >= setup->m_file_data_list[m_file_index].size()) {//dealt last package
            m_last_pack = true;
        }
        QByteArray byte;
        byte.append(CMD_HEAD_OTA);
        byte.append(OTA_SEND_BODY);
        byte.append((char)0x00);
        const QByteArray &data = setup->m_file_data_list.at(m_file_index).mid(i, m_device_mtu);
        m_check_sum += CheckSum((uchar*)data.data(), data.size());
        byte.append(data);
        WriteCharacteristic(m_characteristics, byte);
        if (m_package_num >= m_device_prn) {
            if (!WaitOTABodyReply(10))
            {
                printc << "recv d1 02 time out";
                emit disconnectDevice();
                break ;
            }

            if (m_set_offset) {
                i = m_cur_offset - m_device_mtu;
                m_set_offset = false;
            }
        }
        if (m_last_pack) {
            SendCmdKeyData(CMD_HEAD_OTA, OTA_SEND_END);
            break ;
        }
    }
}

void Service::StopSendData()
{
    m_last_pack = false;
    m_package_num = 0;
    m_file_offset = 0;
    m_check_sum = 0;
}

uchar Service::getFileType(int index)
{
    uchar c = 0xff;
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

bool Service::WaitOTABodyReply(int secTimeout)
{
    bool reply_succ = true;

    QEventLoop eventloop;
    connect(this, &Service::recvOTABodyReply, &eventloop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &eventloop, [&eventloop,&reply_succ]{reply_succ=false; eventloop.quit();});
    timer.start(secTimeout * 1000);

    eventloop.exec();
    timer.stop();
    return reply_succ;
}

int Service::CompareVersion(const QByteArray &version1, const QByteArray &version2)
{
    QByteArrayList alist = version1.split('.');
    QByteArrayList blist = version2.split('.');
    int minLength = qMin(alist.size(), blist.size());
    int idx = 0;
    int diff = 0;
    while (idx < minLength
           && (diff = alist[idx].length() - blist[idx].length()) == 0
           && (diff = alist[idx].compare(blist[idx])) == 0) {
        ++ idx;
    }
    diff = (diff != 0) ? diff : alist.length() - blist.length();
    return diff;
}
