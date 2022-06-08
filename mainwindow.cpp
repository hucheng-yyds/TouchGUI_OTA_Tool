#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "agent.h"
#include "controller.h"
#include "service.h"
#include "device.h"
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

//#define SAMPLE
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , agent(new Agent(this))
    , m_timer(new QTimer(this))
{
    ui->setupUi(this);
    QString qss = ("QPushButton{"                           //正常状态样式
                   "background-color:rgb(60, 179, 113);"    //背景色（也可以设置图片）
                   "border-style:outset;"                   //边框样式（inset/outset）
                   "border-width:4px;"                      //边框宽度像素
                   "border-radius:10px;"                    //边框圆角半径像素
                   "border-color:rgba(255,255,255,30);"     //边框颜色
                   "font:bold 18px;"                        //字体，字体大小
                   "color:rgb(255,255,255);"                //字体颜色
                   "padding:6px;"                           //填衬
                   "}"
                   "QPushButton:hover{"                     //鼠标悬停样式
                   "background-color:rgb(0,139,0);"
                   "border-color:rgba(255,255,255,200);"
                   "color:rgb(255,255,255);"
                   "}"
                   "QPushButton:pressed{"                   //鼠标按下样式
                   "background-color:rgba(100,255,100,200);"
                   "border-color:rgba(255,255,255,30);"
                   "border-style:inset;"
                   "color:rgba(0,0,0,100);"
                   "}"
                   "QTabBar::tab {min-width:100px;background-color:rgb(156,156,156);color: white;"
                   "border-top-left-radius: 10px;border-top-right-radius: 10px;padding:5px;}"
                   "QTabBar::tab:selected {color:rgb(0,139,0);background-color:rgb(207,207,207)}"
                   "QTabBar::tab:hover{""background-color:rgb(207,207,207);"
                   "border-color:rgba(255,255,255,200);""color:rgb(0,139,0);"
                   "}");
    setStyleSheet(qss);
    setWindowTitle("OTA升级工具" + buildDateTime());
    ui->tableWidget_2->setColumnWidth(0, 200);
#ifndef SAMPLE
    connect(agent, &Agent::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(agent, &Agent::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::onUpdateTime));

    qDebug() << "main thread:" << QThread::currentThreadId();
    ui->label_19->installEventFilter(this);
//    agent->startScanDevice(10000, (QStringList()/*<<"3F:E1"*/<<"3D:D8"<<"57:E7"<<"6E:E9"<<"40:E8"));
    QFile file(QCoreApplication::applicationDirPath() + "/setup.txt");
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream in(&file);
        QString string;
        while (in.readLineInto(&string)) {
            qDebug() << string;
            if (!string.indexOf("dir:")) {
                string.remove(0, 4);
                ui->label_19->setText(string);
            } else if (!string.indexOf("ver:")) {
                string.remove(0, 4);
                ui->label_20->setText(string);
                ui->label_31->setText(string);
                ui->label_38->setText(string);
                m_version = string.toUtf8();
                m_version.resize(12);
            } else {
                m_address_list.append(string);
            }
        }
        file.close();
    }
//    if (ui->label_19->text().isEmpty()) {//如果配置文件没有指定路径，那么默认当前目录 oy28_ota_file
//        ui->label_19->setText("oy28_ota_file");
//    }
    GetDirectoryFile(ui->label_19->text());
#else
    device = new Device;
#endif
}

MainWindow::~MainWindow()
{
    on_pushButton_6_clicked();
    delete ui;
}

void MainWindow::GetDirectoryFile(const QString &dirName)
{
    qDebug() << "GetDirectoryFile:" << dirName;
    if(dirName.isEmpty()) {
        return ;
    }
    QDir dir(dirName);
    ui->label_19->setText(QDir::current().relativeFilePath(dirName));//显示相对路径
    ui->label_30->setText(ui->label_19->text());
    ui->label_39->setText(ui->label_19->text());
    QStringList nameFilters("*.bin");
    QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    m_total_file_size = 0;
    m_file_name_list.clear();
    m_file_data_list.clear();
    foreach(QString name, files) {
        QFile file(dir.absoluteFilePath(name));
        qDebug() << "file name:" << file.fileName();
        m_total_file_size += file.size();
        if (file.open(QIODevice::ReadOnly)) {
            m_file_name_list.append(name.toUtf8());
            m_file_data_list.append(file.readAll());
            file.close();
        } else {
            qWarning() << "file cannot open";
        }
    }
}

QString MainWindow::buildDateTime() const
{
    QString dateTime;
    dateTime += __DATE__;
    dateTime += __TIME__;
    dateTime.replace("  "," 0");//注意是两个空格，用于日期为单数时需要转成“空格+0”
    return QLocale(QLocale::English).toDateTime(dateTime, "MMM dd yyyyhh:mm:ss").toString(" yyyy.MM.dd");
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (ui->label_19 == obj
            && ev->type() == QEvent::MouseButtonDblClick) {
        QString dirName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                            QCoreApplication::applicationDirPath(),
                                                            QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);
        GetDirectoryFile(dirName);
        return true;
    }  else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, ev);
    }
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!ui->tableWidget_2->findItems(info.address().toString()
                                      , Qt::MatchFixedString).empty()) {
        qDebug() << "repeat connect";
        return ;
    }
    if (controller_list.size() >= 7) {
        qDebug() << "wait controller_list.size():" << controller_list.size();
        return ;
    }

    Controller *controller = new Controller;
    connect(controller, &Controller::upgradeResult, this, &MainWindow::onUpgradeResult);
    connect(this, &MainWindow::ConnectDevice, controller, &Controller::ConnectDevice);
//    controller->start();
    controller_list.append(controller);
    controller->SetProperty(m_file_data_list, m_file_name_list, m_total_file_size, m_version);
//    controller->ConnectDevice(info);
    emit ConnectDevice(info);
    disconnect(this, &MainWindow::ConnectDevice, controller, &Controller::ConnectDevice);
    int rowCount = ui->tableWidget_2->rowCount();
    ui->tableWidget_2->insertRow(rowCount);//添加到正在升级列表
    ui->tableWidget_2->setItem(rowCount, 0, new QTableWidgetItem(info.address().toString()));
}

void MainWindow::onUpgradeResult(bool success, const QString &address)
{
    QList<QTableWidgetItem*> list = ui->tableWidget_2->findItems(address, Qt::MatchFixedString);
    if (!list.isEmpty()) {//单个升级完成
        int index = ui->tableWidget_2->row(list[0]);
        if (-1 == index) {
            qDebug() << "dont find device";
            index = 0;
        }
        ui->tableWidget_2->removeRow(index);//从正在升级列表移除
        qDebug() << address << "list size:" << list.size() << index;
        delete controller_list[index];
        controller_list.removeAt(index);
        if (success) {
            ui->listWidget->addItem(address);//添加到升级成功列表
            ui->label_33->setText(QString::number(ui->listWidget->count()));
        } else {
            int failCount = ui->label_47->text().toInt();
            qDebug() << "failCount:" << failCount;
            ui->label_47->setText(QString::number(++failCount));//未完成数
        }
    }
    onScanFinished();
}

void MainWindow::onUpdateTime()
{
    QTime time(0, 0);
    time = time.addSecs(m_elapsed_second++);
    ui->label_34->setText(time.toString("hh:mm:ss"));
}

void MainWindow::onScanFinished()
{
    if (controller_list.isEmpty() && !agent->isActive()) {//全部升级完成
        m_timer->stop();
        agent->stopScan();
        ui->tabWidget->setCurrentIndex(5);
        ui->label_46->setText(QString::number(ui->listWidget->count()));//已完成数
        ui->label_49->setText(ui->label_34->text());//总耗时
        double hourCount = ui->listWidget->count() / (m_elapsed_second / 3600.00);
        qDebug() << "hourCount:" << hourCount << m_elapsed_second << ui->listWidget->count();
        ui->label_48->setText(QString::number(hourCount, 'f', 0)+" units/hour");//平均速度
        ui->pushButton_4->setEnabled(true);
    }
}

void MainWindow::on_pushButton_4_clicked()
{
#ifndef SAMPLE
//    agent->startScanDevice(10000, (QStringList()<<"3D:D8"<<"3F:E1"<<"57:E7"<<"6E:E9"<<"40:E8"));
    if (0 == m_total_file_size) {
        QMessageBox::information(this, "提示",
                                 "请检查升级包!",
                                 QMessageBox::NoButton);
        return ;
    }
    if (ui->label_20->text().isEmpty()) {
        if (QMessageBox::question(this, "提示",
                                  "版本号为空，确定要升级吗?")
                == QMessageBox::No) {
            return ;
        }
    }
    agent->setMatchStr(ui->lineEdit_5->text());
    ui->label_32->setText(QString::number(m_address_list.size()));
    ui->label_40->setText(QString::number(m_address_list.size()));//目标总数
    ui->tabWidget->setCurrentIndex(4);
    m_timer->start(1000);
    ui->pushButton_4->setEnabled(false);
    agent->startScanDevice(60 * 1000, m_address_list);
#else
    device->startDeviceDiscovery();
#endif
}


void MainWindow::on_pushButton_3_clicked()
{

}


void MainWindow::on_pushButton_6_clicked()
{
#ifndef SAMPLE
    m_elapsed_second = 0;
    ui->tableWidget_2->clearContents();
    ui->tableWidget_2->setRowCount(0);
    ui->listWidget->clear();
    agent->stopScan();
//    for (auto &cont : controller_list) {
//        delete cont;
//        cont = nullptr;
//    }
    qDeleteAll(controller_list);
    controller_list.clear();
    ui->pushButton_4->setEnabled(true);
#else
    device->disconnectFromDevice();
#endif
}
