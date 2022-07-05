#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QLowEnergyService>
#include <QEventLoop>
#include <QThread>

#define CMD_HEAD_PARAM      (uchar)0x02
#define PARAM_GET_INFO      (uchar)0x01
#define PARAM_GET_VER       (uchar)0x11
#define PARAM_GET_MTU       (uchar)0xF0

#define CMD_HEAD_SETUP      (uchar)0x03
#define SETUP_AUTH          (uchar)0xE7

#define CMD_HEAD_OTA        (uchar)0XD1
#define OTA_SEND_START      (uchar)0x01
#define OTA_SEND_BODY       (uchar)0x02
#define OTA_SEND_END        (uchar)0x03
#define OTA_SET_OFFSET      (uchar)0x04
#define OTA_SET_PRN         (uchar)0x05
#define OTA_GET_OFFSET      (uchar)0x06
#define OTA_SET_PROGRESS    (uchar)0x07
//status code
#define CODE_SKIP_FILE      (uchar)0x7E
#define CODE_SKIP_HEAD      (uchar)0x7F

#define CMD_HEAD_SYSTEM     (uchar)0xF0
#define SYSTEM_REBOOT       (uchar)0x01
#define SYSTEM_DISCONNECT   (uchar)0x02
#define SYSTEM_POWER_OFF    (uchar)0x03

class Service : public QObject
{
    Q_OBJECT
public:
    explicit Service(QObject *parent = nullptr);
    ~Service() {
        SendMessage("~Service");
    }

    void SetProperty(QByteArrayList &data, QByteArrayList &name, int size, QByteArray &version);
    void ConnectService(QLowEnergyService *, const QString &address);
    void SendMessage(const QString &);
    void OpenNotify(QLowEnergyCharacteristic ch, bool flag);
    void ReadCharacteristic(QLowEnergyCharacteristic ch);
    void WriteCharacteristic(QLowEnergyCharacteristic ch, const QByteArray &arr);

    uchar getFileType(int index);
    uint32_t CheckSum(uint8_t *pBuffer, uint8_t len);

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
    void message(QString);
    void discoveryCharacteristic(QLowEnergyCharacteristic);
    void disconnectDevice();
    void upgradeResult(bool success, const QString &address);

private:
    void SendCmdKeyData(const uchar cmd, const uchar key);
    void StartSendData();
    void StopSendData();
    bool WaitReplyData(int secTimeout);
    bool WaitReplyData2(int secTimeout);
    int CompareVersion(const QByteArray &version1, const QByteArray &version2);

    QLowEnergyService * m_service;
    QLowEnergyCharacteristic m_characteristics;
    QString m_address;
    QEventLoop *m_eventloop;
    QTimer *m_timer;

    QByteArrayList m_file_data_list;
    QByteArrayList m_file_name_list;
    int m_total_file_size = 0;
    QByteArray m_version;

    bool m_last_pack = false;
    bool m_set_offset = false;
    int m_device_mtu = 241;
    int m_device_prn = 100;
    int m_file_index = 0;
    int m_file_offset = 0;
    int m_cur_offset = 0;
    int m_package_num = 0;
    int m_check_sum = 0;
    int m_cur_sum = 0;
};

#endif // SERVICE_H
