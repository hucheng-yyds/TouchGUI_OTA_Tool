#include "setup.h"
#include "log.h"
#include <QFile>
#include <QTextStream>
#include <QtDebug>

Setup* Setup::m_Instance = nullptr;

Setup::Setup(QObject *parent)
    : QObject{parent}
{
    QFile file("setup.txt");
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream in(&file);
        QString string, line;
        while (in.readLineInto(&line)) {
            string = line;
            if (!string.indexOf("#")
                    || string.isEmpty()) {
                continue ;
            } else if (!string.indexOf("dir:")) {
                string.remove(0, 4);
                m_dirName = string;
            } else if (!string.indexOf("ver:")) {
                string.remove(0, 4);
                m_version = string.toUtf8();
                m_version.resize(12);
            } else if (!string.indexOf("queue:")){
                string.remove(0, 6);
                m_queuemax = string.toInt();
            } else if (!string.indexOf("loglevel:"))
            {
                string.remove(0, 9);
                Log::m_LogLevel = string.toInt();
            } else if (!string.indexOf("server:"))
            {
                string.remove(0, 7);
                m_serverIndex = string.toInt();
            } else if (!string.indexOf("poweroff:"))
            {
                string.remove(0, 9);
                if (string.toInt()>0)
                {
                    m_ota_poweroff = true;
                }
            } else if (!string.indexOf("local:"))
            {
                string.remove(0, 6);
                if (string.toInt()>0)
                {
                    m_ota_local = true;
                    qInfo() << "set ota local:true";
                }
            }
            else {
                m_address_list.append(string);
            }
            qInfo() << line;
        }

        file.close();
    }

    //add all fail mac addresses
    QFile failfile("fail_mac.txt");
    if (failfile.open(QIODevice::ReadOnly)) {
        QTextStream in(&failfile);
        QString string;
        while (in.readLineInto(&string)) {
            if (!string.indexOf("#")
                    || string.isEmpty()) {
                continue;
            }
            m_address_list.append(string);
            qInfo() << "add fai mac:" << string;
        }
        failfile.close();
        failfile.remove();
    }

    //添加扫描枪录入的mac地址
    QFile macfile("mac.txt");
    if (macfile.open(QIODevice::ReadOnly)) {
        QTextStream in(&macfile);
        QString string;
        while (in.readLineInto(&string)) {
            if (!string.indexOf("#")
                    || string.isEmpty()) {
                continue;
            }
            m_address_list.append(string);
            qInfo() << "add QR gun mac:" << string;
        }
        macfile.close();
    }

    //去重
    m_address_list.removeDuplicates();

    //删除上次OTA成功的mac
    QFile succfile("success_mac.txt");
    if (succfile.open(QIODevice::ReadOnly)) {
        QTextStream in(&succfile);
        QString string;
        while (in.readLineInto(&string)) {
            if (!string.indexOf("#")
                    || string.isEmpty()) {
                continue;
            }
            m_address_list.removeOne(string);
            qInfo() << "delete succ mac:" << string;
        }
        succfile.close();
    }
}

Setup::~Setup()
{
#if QT_NO_DEBUG
    writeSuccessMac();
#endif
    writeFailMac();
}

void Setup::writeMac()
{
    QFile file("mac.txt");
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    for (auto &a : m_address_list)
    {
        out << a << '\n';
    }
    file.close();
}

void Setup::writeSuccessMac()
{
    QFile file("success_mac.txt");
    if (file.open(QIODevice::WriteOnly|QIODevice::Append))
    {
        QTextStream out(&file);
        for (auto &a : m_success_address_list)
        {
            out << a << '\n';
        }
        file.close();
    }
}

void Setup::writeFailMac()
{
    QFile file("fail_mac.txt");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        for (auto &a : m_fail_address_list)
        {
            out << a << '\n';
        }
        file.close();
    }
}
