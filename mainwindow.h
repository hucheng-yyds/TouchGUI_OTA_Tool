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
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onUpgradeResult(bool success, const QString &address);
    void onUpdateTime();
    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();
signals:
    void ConnectDevice(const QBluetoothDeviceInfo &info);

private:
    Ui::MainWindow *ui;
    Agent *agent;
//    Controller *controller;
//    Service *service;
    Device *device;
    QList<Controller *> controller_list;
    QByteArrayList m_file_data_list;
    QByteArrayList m_file_name_list;
    int m_total_file_size = 0;
    QTimer *m_timer;
    int m_elapsed_second = 0;
};
#endif // MAINWINDOW_H
