#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "agent.h"
#include "controller.h"
#include "service.h"
#include "device.h"
#include <QFile>
#include <QDir>
#include <QTime>
#include <QFileDialog>

//#define SAMPLE
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , agent(new Agent(this))
    , m_timer(new QTimer(this))
{
    ui->setupUi(this);
#ifndef SAMPLE
    connect(agent, &Agent::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::onUpdateTime));

    qDebug() << "main thread:" << QThread::currentThreadId();
    ui->label_19->installEventFilter(this);
    ui->label_19->setText(QCoreApplication::applicationDirPath() + "/oy28_ota_file");
//    agent->startScanDevice(10000, (QStringList()/*<<"3F:E1"*/<<"3D:D8"<<"57:E7"<<"6E:E9"<<"40:E8"));
    ui->lineEdit_4->setText("3D:D8;57:E7;6E:E9;40:E8");
#else
    device = new Device;
#endif
}

MainWindow::~MainWindow()
{
    on_pushButton_3_clicked();
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (ui->label_19 == obj
            && ev->type() == QEvent::MouseButtonDblClick) {
        QString dirString = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                              QCoreApplication::applicationDirPath(),
                                                              QFileDialog::ShowDirsOnly
                                                              | QFileDialog::DontResolveSymlinks);
        qDebug() << dirString;
        QDir dir(dirString);
        QStringList nameFilters("*.bin");
        QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
        foreach(QString name, files) {
            QFile file(dir.absoluteFilePath(name));
            qDebug() << "file name:" << file.fileName();
            m_total_file_size += file.size();
            if (file.open(QIODevice::ReadOnly)) {
                m_file_name_list.append(name.toUtf8());
                m_file_data_list.append(file.readAll());
                file.close();
                ui->pushButton_4->setEnabled(true);
            } else {
                qWarning() << "file cannot open";
            }
        }
        return true;
    }  else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, ev);
    }
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    Controller *controller = new Controller;
    connect(controller, &Controller::upgradeResult, this, &MainWindow::onUpgradeResult);
    connect(this, &MainWindow::ConnectDevice, controller, &Controller::ConnectDevice);
//    controller->start();
    controller_list.append(controller);
    controller->SetProperty(m_file_data_list, m_file_name_list, m_total_file_size);
//    controller->ConnectDevice(info);
    emit ConnectDevice(info);
    disconnect(this, &MainWindow::ConnectDevice, controller, &Controller::ConnectDevice);
    int rowCount = ui->tableWidget_2->rowCount();
    ui->tableWidget_2->insertRow(rowCount);
    ui->tableWidget_2->setItem(rowCount, 0, new QTableWidgetItem(info.address().toString()));
}

void MainWindow::onUpgradeResult(bool success, const QString &address)
{
    QList<QTableWidgetItem*> list = ui->tableWidget_2->findItems(address, Qt::MatchFixedString);
    if (!list.isEmpty()) {
        int index = ui->tableWidget_2->row(list[0]);
        ui->tableWidget_2->removeRow(index);
        qDebug() << address << "list size:" << list.size() << index;
        delete controller_list[index];
        controller_list.removeAt(index);
        if (success) {
            ui->listWidget->addItem(address);
            ui->label_33->setText(QString::number(ui->listWidget->count()));
        } else {
            int failCount = ui->label_47->text().toInt();
            qDebug() << "failCount:" << failCount;
            ui->label_47->setText(QString::number(++failCount));
        }
    }
    if (controller_list.isEmpty()) {
        m_timer->stop();
        ui->tabWidget->setCurrentIndex(5);
        ui->label_46->setText(QString::number(ui->listWidget->count()));
        ui->label_49->setText(ui->label_34->text());
        double hourCount = ui->listWidget->count() / (m_elapsed_second / 3600.00);
        qDebug() << hourCount << m_elapsed_second << ui->listWidget->count();
        ui->label_48->setText(QString::number(hourCount, 'f', 0)+" units/hour");
    }
}

void MainWindow::onUpdateTime()
{
    QTime time(0, 0);
    time = time.addSecs(m_elapsed_second++);
    ui->label_34->setText(time.toString("hh:mm:ss"));
}

void MainWindow::on_pushButton_4_clicked()
{
#ifndef SAMPLE
//    agent->startScanDevice(10000, (QStringList()<<"3D:D8"<<"3F:E1"<<"57:E7"<<"6E:E9"<<"40:E8"));
    agent->startScanDevice(10000, ui->lineEdit_4->text().split(';'));
    ui->tabWidget->setCurrentIndex(4);
    m_timer->start(1000);
#else
    device->startDeviceDiscovery();
#endif
}


void MainWindow::on_pushButton_3_clicked()
{
#ifndef SAMPLE
    m_elapsed_second = 0;
    ui->tableWidget_2->clearContents();
    ui->tableWidget_2->setRowCount(0);
    ui->listWidget->clear();
//    for (auto &cont : controller_list) {
//        delete cont;
//        cont = nullptr;
//    }
    qDeleteAll(controller_list);
    controller_list.clear();
#else
    device->disconnectFromDevice();
#endif
}

