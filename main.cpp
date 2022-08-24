#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include "log.h"

int main(int argc, char *argv[])
{
#if 0
    qInfo() << "start app";
    QProcess *m_process = new QProcess;
    while (1) {
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        m_process->start("cmd", QStringList()<<"/c"<<"tasklist | findstr TouchGUI_OTA_Tool1");
        m_process->waitForFinished(3000);
        QString strRet = m_process->readLine();
        if(strRet.isEmpty()){
            m_process->start("TouchGUI_OTA_Tool1.exe", QStringList());
            qInfo() << "start exe";
            if (m_process->waitForFinished(-1)) {
                qInfo() << "quit process" << m_process->exitStatus() << m_process->exitCode();
                if (m_process->exitStatus() == QProcess::NormalExit) {
                    break;
                }
            }
        }
    }
    m_process->close();
    delete m_process;
    return 0;
#endif
    QApplication a(argc, argv);
    Log l;
    MainWindow w;
    w.show();
    return a.exec();
}
