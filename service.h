#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QLowEnergyService>
#include <QCryptographicHash>

#define CMD_HEAD_PARAM  (uchar)0x02
#define KEY_GET_INFO    (uchar)0x01
#define KEY_GET_MTU     (uchar)0xF0

#define CMD_HEAD_OTA    (uchar)0XD1
#define CMD_SEND_START   (uchar)0x01
#define CMD_SEND_BODY    (uchar)0x02
#define CMD_SEND_END     (uchar)0x03
#define CMD_SET_OFFSET   (uchar)0x04
#define CMD_SET_PRN      (uchar)0x05
#define CMD_GET_OFFSET   (uchar)0x06
#define CMD_SET_PROGRESS (uchar)0x07

class Service : public QObject
{
    Q_OBJECT
public:
    explicit Service(QObject *parent = nullptr);

    void ConnectService(QLowEnergyService *);
    void SendMessage(QString);
    void OpenNotify(QLowEnergyCharacteristic ch, bool flag);
    void ReadCharacteristic(QLowEnergyCharacteristic ch);
    void WriteCharacteristic(QLowEnergyCharacteristic ch, const QByteArray &arr);
    void SendCmdKeyData(const uchar cmd, const uchar key, QByteArray &byte);
    void StartSendData();
    void StopSendData();

    uchar getFileType(int index);
    uint32_t CheckSum(uint8_t *pBuffer, uint8_t len);
    QByteArrayList m_file_data_list;
    QByteArrayList m_file_name_list;
    int m_total_file_size = 0;

private slots:
    void onStateChanged(QLowEnergyService::ServiceState newState);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &info,
                                 const QByteArray &value);
    void onCharacteristicRead(const QLowEnergyCharacteristic &info,
                              const QByteArray &value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &info,
                                 const QByteArray &value);
    void onDescriptorRead(const QLowEnergyDescriptor &info,
                          const QByteArray &value);
    void onDescriptorWritten(const QLowEnergyDescriptor &info,
                             const QByteArray &value);
    void onError(QLowEnergyService::ServiceError error);

signals:
    void discoveryCharacteristic(QLowEnergyCharacteristic);
    void message(QString);

private:
    QLowEnergyService * m_service;
    QList<QLowEnergyCharacteristic> m_characteristics_list;
    int m_device_mtu = 241;
    int m_device_prn = 100;
    int m_file_index = 1;
    int m_file_offset = 0;
    int m_package_num = 0;
    int m_check_sum = 0;
    int m_cur_sum = 0;
    int m_timeout_num = 3;

};

#endif // SERVICE_H
