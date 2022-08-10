#ifndef LOG_H
#define LOG_H

#include <QObject>
#include <QtDebug>

class Log : public QObject
{
    Q_OBJECT
public:
    explicit Log(QObject *parent = nullptr);

    static QtMsgType m_LogLevel;
    static void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
signals:

};

#endif // LOG_H
