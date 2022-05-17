#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "agent.h"
#include "controller.h"
#include "service.h"
#include "device.h"
#include <QTimer>
#include <QFile>
#include <QDir>

//#define SAMPLE
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
//    QByteArray arr;
//    arr.append(0x7b);
//    arr.append(0xc1);
//    arr.append(0x01);
//    arr.append((char)0x00);
//    int value = (int)((arr[0] & 0xFF)
//            | ((arr[1] & 0xFF) << 8)
//            | ((arr[2] & 0xFF) << 16)
//            | ((arr[3] & 0xFF) << 24));
//    qDebug() << value;
//    return ;
    ui->setupUi(this);
#ifndef SAMPLE
    agent = new Agent;
    controller = new Controller;
    service = new Service;
    connect(agent, &Agent::deviceDiscovered, controller, &Controller::ConnectDevice);
//    connect(agent, &Agent::deviceDiscovered, this, [this](const QBluetoothDeviceInfo &info) {
//        controller->ConnectDevice(info);
//    });
    connect(controller, &Controller::serviceDiscovered, this, [this](const QBluetoothUuid &newUuid) {
        uuid = newUuid;
        QTimer::singleShot(500, this, [this]() {
            qDebug() << "uuid:" << uuid;
            service->ConnectService(controller->CreateService(uuid));
        });
    });
    connect(service, &Service::discoveryCharacteristic, this, []() {
//        service->WriteCharacteristic(ch)
    });
    QDir dir(QCoreApplication::applicationDirPath() + "/oy28_ota_file");
    QStringList nameFilters;
    nameFilters << "*.bin";
    QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    foreach(QString name, files) {
        QFile file(dir.absoluteFilePath(name));
        qDebug() << "file name:" << file.fileName();
        service->m_total_file_size += file.size();
        if (file.open(QIODevice::ReadOnly)) {
            service->m_file_name_list.append(name.toUtf8());
            service->m_file_data_list.append(file.readAll());
            file.close();
        } else {
            qWarning() << "file cannot open";
        }
    }
    agent->startScanDevice(5000, "3D:D8");
#else
    device = new Device;
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_4_clicked()
{
#ifndef SAMPLE
    agent->startScanDevice(5000, "3D:D8");
#else
    device->startDeviceDiscovery();
#endif
}


void MainWindow::on_pushButton_3_clicked()
{
#ifndef SAMPLE
    controller->DisconnectDevice();
#else
    device->disconnectFromDevice();
#endif
}

