#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyService>
#include <QTimer>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Agent;
class Controller;
class Service;
class Device;
class HttpsClient;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void GetDirectoryFile(const QString &dirName);
    QString buildDateTime() const;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void keyPressEvent(QKeyEvent *keyValue) override;

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onUpgradeResult(bool success, const QString &address);
    void onUpdateTime();

    void on_pushButton_4_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_6_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

signals:
    void ConnectDevice(const QBluetoothDeviceInfo &info, int timeout);
    void stopAgentScan();
    void startAgentScan();

private:
    void checkScan();

private:
    Ui::MainWindow *ui;
    Agent *agent;
    HttpsClient *https;
    QList<Controller *> controller_list;
    QByteArrayList m_file_data_list;
    QByteArrayList m_file_name_list;
    int m_total_file_size = 0;
    QByteArray m_version;
    QTimer *m_timer;
    int m_elapsed_second = 0;
    QStringList m_address_list;
    QString m_barStr;

    int m_targetcount = 0;
    int m_successcount = 0;
    int m_failcount = 0;
    int m_queuemax = 2;

    //controller start time out, seconds
    int m_startTimeout = 30;

    //agent scan timeout
    int m_scanTimeout = 600;
};
#endif // MAINWINDOW_H
