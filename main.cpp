#include "mainwindow.h"
#include <QApplication>
#include <QTime>
#include <QTimer>
#include <QFile>
#include <QMutex>
#include <QTextCodec>
#include <stdio.h>
#include <QThread>

static QtMsgType g_LogLevel = QtWarningMsg;

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
    //FILE * fp = stdout;
    switch (type) {
    case QtDebugMsg:
        strLevel = QString("Debug: ");
        break;
    case QtInfoMsg:
        strLevel = QString("Info: ");
        break;
    case QtWarningMsg:
        strLevel = QString("Warning: ");
        //fp = stderr;
        break;
    case QtCriticalMsg:
        strLevel = QString("Critical: ");
        //fp = stderr;
        break;
    case QtFatalMsg:
        strLevel = QString("Fatal: ");
        //fp = stderr;
        break;
    default:
        strLevel = QString("Default: ");
        break;
    }

    //win console printer
//    fprintf(fp, "%s%s %s\n",
//            strLevel.toStdString().c_str(),
//            strDate.toStdString().c_str(),
//            msg.toStdString().c_str());
//    fflush(fp);

    //write msg to log file
    static QMutex m;
    m.lock();
    QFile logfile("ota.log");
    bool ret = logfile.open(QIODevice::ReadWrite | QIODevice::Append);
    if (ret){
        QTextStream out(&logfile);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << strLevel << strDate << " "
            << QThread::currentThreadId() << " "
            << msg
            << "\n";
        logfile.flush();
        logfile.close();
    }
    m.unlock();
}

QtMsgType convertLogLevel(int level)
{
    QtMsgType ret = QtWarningMsg;
    switch (level) {
    case 0:
        ret = QtDebugMsg;
        break;
    case 1:
        ret = QtWarningMsg;
        break;
    case 2:
        ret = QtCriticalMsg;
        break;
    case 3:
        ret = QtFatalMsg;
        break;
    case 4:
        ret = QtInfoMsg;
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
        while (in.readLineInto(&string)) {
            if (!string.indexOf("loglevel:")){
                string.remove(0, 9);
                g_LogLevel = convertLogLevel(string.toInt());
                break;
            }
        }
        file.close();
    }
    qDebug() << "setLogLevel: " << g_LogLevel;
}

void clearFiles()
{
    QFile logfile("ota.log");
    logfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
    logfile.close();

    QFile mfile("success_mac.txt");
    mfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
    mfile.close();
}

int main(int argc, char *argv[])
{
    clearFiles();
    qInstallMessageHandler(myMessageOutput);
    setLogLevel();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
