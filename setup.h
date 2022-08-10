#ifndef SETUP_H
#define SETUP_H

#include <QObject>

#define setup Setup::getInstance()

class Setup : public QObject
{
    Q_OBJECT
    static Setup *m_Instance;
public:
    explicit Setup(QObject *parent = nullptr);
    ~Setup();

    static Setup *getInstance() {
        if (!m_Instance) {
            m_Instance = new Setup;
        }
        return m_Instance;
    }
    void writeMac();
    void writeSuccessMac();
    void writeFailMac();

    QStringList m_address_list;
    QStringList m_success_address_list;
    QStringList m_fail_address_list;
    QByteArrayList m_file_data_list;
    QByteArrayList m_file_name_list;
    QByteArray m_version;
    QString m_dirName;

    //ota结束后是否关机
    bool m_ota_poweroff = false;
    bool m_ota_local = false;
    bool m_ignore_version_compare = false;

    int m_total_file_size = 0;
    int m_queuemax = 8;

    //http服务环境配置
    int m_serverIndex = 0;

    //controller start time out, seconds
    int m_startTimeout = 30;

    //agent scan timeout
    int m_scanTimeout = 120;

signals:

};

#endif // SETUP_H
