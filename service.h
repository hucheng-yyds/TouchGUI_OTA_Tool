#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QLowEnergyService>
#include <QEventLoop>
#include <QThread>
#include <QTimer>

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
    enum ResultState {
        SuccessOTA = 0,
        HighVersion,
        LowEnergy,
        ConnectionException
    };
    Q_ENUM(ResultState)

    explicit Service(QObject *parent = nullptr);
    ~Service();

    void ConnectService(QLowEnergyService *, const QString &address);
    void OpenNotify(const QLowEnergyCharacteristic &ch, bool flag);
    void ReadCharacteristic(const QLowEnergyCharacteristic &ch);
    void WriteCharacteristic(const QLowEnergyCharacteristic &ch, const QByteArray &arr);

    QString getVersion() const;
    uchar getFileType(int index);
    uint32_t CheckSum(uint8_t *pBuffer, uint8_t len);

private slots:
    void onStateChanged(QLowEnergyService::ServiceState newState);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &info,
                                 const QByteArray &value);
#ifdef QT_DEBUG
    void onCharacteristicRead(const QLowEnergyCharacteristic &info,
                              const QByteArray &value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &info,
                                 const QByteArray &value);
    void onDescriptorRead(const QLowEnergyDescriptor &info,
                          const QByteArray &value);
    void onDescriptorWritten(const QLowEnergyDescriptor &info,
                             const QByteArray &value);
#endif
    void onError(QLowEnergyService::ServiceError error);

signals:
    void upgradeResult(Service::ResultState state);
    void startOTA();
    void recvOTABodyReply();

private:
    void SendCmdKeyData(const uchar cmd, const uchar key);
    void StartSendData();
    void StopSendData();
    bool WaitReplyData(int secTimeout);
    bool WaitOTABodyReply(int secTimeout);
    int CompareVersion(const QByteArray &version1, const QByteArray &version2);

    QLowEnergyService *m_service = nullptr;
    QLowEnergyCharacteristic m_characteristics;
    QByteArray m_oy22b_tpversion;
    QString m_address;
    QString m_version;

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
    bool m_ota_finished = false;
};

#endif // SERVICE_H
