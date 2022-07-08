#include "mainwindow.h"
#include <QApplication>
#include <QTime>
#include <QTimer>
#include <QFile>
#include <QMutex>
#include <QTextCodec>
#include <stdio.h>

static QtMsgType g_LogLevel = QtDebugMsg;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type < g_LogLevel)
    {
        return;
    }

    //const char *file = context.file ? context.file : "";
    //const char *function = context.function ? context.function : "";

    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz");
    QString strLevel;
    FILE * fp = stdout;
    switch (type) {
    case QtDebugMsg:
        strLevel = QString("Debug: ");
        break;
    case QtInfoMsg:
        strLevel = QString("Info: ");
        break;
    case QtWarningMsg:
        strLevel = QString("Warning: ");
        fp = stderr;
        break;
    case QtCriticalMsg:
        strLevel = QString("Critical: ");
        fp = stderr;
        break;
    case QtFatalMsg:
        strLevel = QString("Fatal: ");
        fp = stderr;
        break;
    default:
        strLevel = QString("Default: ");
        break;
    }

    QByteArray localMsg = msg.toLocal8Bit();
    //win console printer
    fprintf(fp, "%s%s %s\n",
            strLevel.toStdString().c_str(),
            strDate.toStdString().c_str(), localMsg.constData());

    //write msg to log file
    static QMutex m;
    m.lock();
    QFile logfile("ota.log");
    bool ret = logfile.open(QIODevice::ReadWrite | QIODevice::Append);
    if (ret){
        QTextStream out(&logfile);
        out.setCodec(QTextCodec::codecForName("utf-8"));
        out << strLevel << strDate << " "
            << localMsg.constData()
            << "\n";
        logfile.flush();
        logfile.close();
    }
    m.unlock();
}

QtMsgType convertLogLevel(int level)
{
    QtMsgType ret = QtDebugMsg;
    switch (level) {
    case 0:
        ret = QtDebugMsg;
        break;
    case 1:
        ret = QtInfoMsg;
        break;
    case 2:
        ret = QtWarningMsg;
        break;
    case 3:
        ret = QtCriticalMsg;
        break;
    case 4:
        ret = QtFatalMsg;
        break;
    default:
        break;
    }
    return ret;
}

void setLogLevel()
{
    QFile file("setup.txt");
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream in(&file);
        QString string;
        int level = 0;
        while (in.readLineInto(&string)) {
            if (!string.indexOf("loglevel:")){
                string.remove(0, 9);
                level = string.toInt();
                break;
            }
        }
        file.close();
        g_LogLevel = convertLogLevel(level);
    }
    qDebug() << "setLogLevel: " << g_LogLevel;
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    setLogLevel();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
