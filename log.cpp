#include "log.h"
#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QTextCodec>
#include <QThread>

QtMsgType Log::m_LogLevel = QtWarningMsg;

Log::Log(QObject *parent)
    : QObject{parent}
{
    QFile::remove("ota.log");
#ifdef QT_DEBUG
    qSetMessagePattern("%{if-debug}\033[32m%{endif}%{if-critical}\033[31m%{endif}%{if-warning}\033[33m%{endif}%{if-info}\033[34m%{endif}"
                       "[%{type}]\033[0mTime:%{time [yyyy-MM-dd hh:mm:ss zzz]} Tid:%{threadid} Msg:%{message} File:%{file} Line:%{line}");
#else
    qInstallMessageHandler(messageOutput);
#endif
}

void Log::messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type < m_LogLevel)
    {
        return;
    }
    const char *file = context.file ? context.file : "";

    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz");
    QString strLevel;

    switch (type) {
    case QtDebugMsg:
        strLevel = QString("Debug: ");
        break;
    case QtInfoMsg:
        strLevel = QString("Info: ");
        break;
    case QtWarningMsg:
        strLevel = QString("Warning: ");
        break;
    case QtCriticalMsg:
        strLevel = QString("Critical: ");
        break;
    case QtFatalMsg:
        strLevel = QString("Fatal: ");
        break;
    default:
        strLevel = QString("Default: ");
        break;
    }

    static QMutex m;
    m.lock();
    QFile logfile("ota.log");
    bool ret = logfile.open(QIODevice::WriteOnly | QIODevice::Append);
    if (ret) {
        QTextStream out(&logfile);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << strLevel << strDate << ' ' << msg << ' '
            << '(' << file << ':' << context.line << ')'
            << "\n";
        logfile.close();
    }
    m.unlock();
}
