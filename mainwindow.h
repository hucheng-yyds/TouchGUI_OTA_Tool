#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qbluetoothuuid.h"
#include <QMainWindow>

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

private slots:
    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;
    Agent *agent;
    Controller *controller;
    Service *service;
    Device *device;
    QBluetoothUuid uuid;
};
#endif // MAINWINDOW_H
