#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "agent.h"
#include "controller.h"
#include "service.h"
#include "device.h"
#include "httpsclient.h"
#include "setup.h"
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QOperatingSystemVersion>

//#define SAMPLE
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , agent(new Agent(this))
    , https(new HttpsClient(this))
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
    setWindowTitle(QString::fromLocal8Bit("OTA升级工具") + buildDateTime());
    ui->tableWidget_2->setColumnWidth(0, 200);
    ui->tableWidget->setColumnHidden(0, true);
#ifndef SAMPLE
    qInfo() << "main thread" << QThread::idealThreadCount();
    ui->label_19->setText(setup->m_dirName);
    ui->label_20->setText(setup->m_version);
    ui->label_31->setText(setup->m_version);
    ui->label_38->setText(setup->m_version);

    connect(agent, &Agent::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(agent, &Agent::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::onUpdateTime));

    ui->label_19->installEventFilter(this);
    ui->label_52->installEventFilter(this);
    GetDirectoryFile(ui->label_19->text());
#else
    device = new Device;
#endif
    //init vcode
    if (!setup->m_ota_local)
    {
        ui->tabWidget->setCurrentIndex(0);
        refreshVCode();
    }

    //get system version
    auto cur_system = QOperatingSystemVersion::current();
    if (cur_system > QOperatingSystemVersion::Windows10)
    {
        setup->m_startTimeout = 60;
    }

    qInfo() << "current operating system version:" << cur_system;
    qInfo() << "set controller start timeout:" << setup->m_startTimeout << "secs";
}

MainWindow::~MainWindow()
{
    delete setup;
    on_pushButton_6_clicked();
    delete ui;
    delete https;
    delete m_timer;
    delete agent;
}

void MainWindow::GetDirectoryFile(const QString &srcDirName)
{
    qDebug() << "GetDirectoryFile:" << srcDirName;
    if(srcDirName.isEmpty()) {
        return ;
    }

    QDir dir(srcDirName);
    ui->label_19->setText(QDir::current().relativeFilePath(srcDirName));//显示相对路径
    ui->label_30->setText(ui->label_19->text());
    ui->label_39->setText(ui->label_19->text());
    QStringList nameFilters("*.bin");
    QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    setup->m_total_file_size = 0;
    setup->m_file_name_list.clear();
    setup->m_file_data_list.clear();
    foreach(QString name, files) {
        QFile file(dir.absoluteFilePath(name));
        qInfo() << "file name:" << file.fileName();
        setup->m_total_file_size += file.size();
        if (file.open(QIODevice::ReadOnly)) {
            setup->m_file_name_list.append(name.toUtf8());
            setup->m_file_data_list.append(file.readAll());
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

void MainWindow::refreshVCode()
{
    if (https->verificationCode() < 0) {
        QMessageBox::information(this, "Tips", QString::fromLocal8Bit("验证码获取失败"));
    }
    else
    {
        ui->label_52->setPixmap(QPixmap(QString::fromUtf8("a.jpg")));
    }
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
    } else if (ui->label_52 == obj
               && ev->type() == QEvent::MouseButtonPress) {
        refreshVCode();
        return true;
    } else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, ev);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *keyValue)
{
    if (keyValue->key() == Qt::Key_Return) //enter
    {
        if (ui->lineEdit_4->text().isEmpty()) {
            ui->lineEdit_4->setText(m_barStr);
        } else {
            m_barStr = ui->lineEdit_4->text();
        }
        QStringList strlist = m_barStr.split("MAC:");
        for (int i = 1; i < strlist.size(); i ++) {
            bool ok = false;
            //there are two mac address formats
            //abcdef123456, ab:cd:ef:12:34:56
            QString macStr = strlist.at(i).left(12);
            if (macStr.indexOf(":") > 0)
            {
                macStr = strlist.at(i).left(17);
            }
            else
            {
                quint64 macNum = macStr.toULongLong(&ok, 16);
                QBluetoothAddress mac(macNum);
                macStr = mac.toString();
            }
            if (macStr.size() != 17)
            {
                continue;
            }
            qDebug() << macStr;
            setup->m_address_list << macStr;
        }
        setup->m_address_list.removeDuplicates();
        m_targetcount = setup->m_address_list.size();
        ui->lineEdit_4->setText(QString::number(m_targetcount));
        setup->writeMac();
        m_barStr.clear();
    }
    else {
        m_barStr += keyValue->text();
    }
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!ui->tableWidget_2->findItems(info.address().toString()
                                      , Qt::MatchFixedString).empty()) {
        qInfo() << info.address().toString() << "is already connecting";
        return ;
    }
    if (!ui->listWidget->findItems(info.address().toString()
                                      , Qt::MatchFixedString).empty()) {
        qInfo() << info.address().toString() << "has already finished";
        return ;
    }
    Controller *controller = new Controller(info);
    connect(controller, &Controller::upgradeResult, this, &MainWindow::onUpgradeResult);
    controller_list.append(controller);
    if (controller_list.size() >= setup->m_queuemax) {
        agent->stopScan();
    } else if (m_targetcount - m_successcount - m_failcount == controller_list.size()) {
        agent->stopScan();
    }
}

void MainWindow::onScanFinished(bool isTimeout)
{
    if (ui->checkBox->checkState() == Qt::CheckState::Checked)
    {
        setup->m_ignore_version_compare = true;
        qInfo() << "set ignore version compare: true";
    }
    if (setup->m_ota_poweroff)
    {
        qInfo() << "set ota poweroff: true";
    }
    if (isTimeout) {
        if (ui->lineEdit_5->text().isEmpty()) {
            for (const auto &address : qAsConst(setup->m_address_list)) {
                if (!setup->m_fail_address_list.contains(address)
                        && !setup->m_success_address_list.contains(address)) {
                    bool find = false;
                    for (const auto &con : qAsConst(controller_list)) {
                        if (con->address() == address) {
                            find = true;
                            break;
                        }
                    }
                    if (!find) {
                        setup->m_fail_address_list << address;
                        m_failcount ++;
                    }
                }
            }
        } else {
            m_failcount = m_targetcount - m_successcount - controller_list.size();
        }
    }
    while (!controller_list.isEmpty()) {
        qInfo() << "m_processingcount:" << m_processingcount << "setup->m_queuemax:" << setup->m_queuemax;
        addProgressList(controller_list.takeFirst());
    }
    checkScan();
}

void MainWindow::onUpgradeResult(bool success, const QString &address)
{
    QList<QTableWidgetItem*> list = ui->tableWidget_2->findItems(address, Qt::MatchFixedString);
    if (!list.isEmpty()) {//单个升级完成
        int index = ui->tableWidget_2->row(list[0]);
        if (-1 == index) {
            qWarning() << "do not find device:" << address;
            return;
        }
        qInfo() << address << "upgrade result:" << success
                << "row index:" << index;
        if (success) {
            ui->listWidget->addItem(address);//添加到升级成功列表
            ui->label_33->setText(QString::number(ui->listWidget->count()));
            m_successcount++;
            setup->m_success_address_list << address;
            delete controller_progress_list[index];
        }
        else
        {
            if (controller_progress_list[index]->connectCount() < 0) {
                m_failcount++;
                setup->m_fail_address_list << address;
                delete controller_progress_list[index];
            } else {
                controller_list << controller_progress_list[index];
            }
        }
        controller_progress_list.removeAt(index);
        ui->tableWidget_2->removeRow(index);//从正在升级列表移除
        m_processingcount --;
        checkScan();
//        for (int i = 0; i < controller_progress_list.size(); i ++) {
//            qInfo() << "p_list index:" << i
//                    << "p_con address:" << controller_progress_list.at(i)->address();
//        }
        qInfo() << "processing_count:" << m_processingcount
                << "success_count:" << m_successcount
                << "fail_count:" << m_failcount
                << "target_count:" << m_targetcount;
    }
    else
    {
        qWarning() << "do not find device:" << address;
    }
}

void MainWindow::onUpdateTime()
{
    QTime time(0, 0);
    time = time.addSecs(m_elapsed_second++);
    ui->label_34->setText(time.toString("hh:mm:ss"));
}

void MainWindow::checkScan()
{
    if (agent->isFindCountEnough()
            && !controller_list.isEmpty()) {
        addProgressList(controller_list.takeFirst());
    } else if (m_processingcount == 0) {//全部升级完成
        if (m_successcount + m_failcount < m_targetcount) {
            agent->startScan();
            return ;
        }
        m_timer->stop();
        agent->stopScan();
        ui->tabWidget->setCurrentIndex(5);
        ui->label_46->setText(QString::number(ui->listWidget->count()));//已完成数
        ui->label_49->setText(ui->label_34->text());//总耗时
        ui->label_47->setText(QString::number(m_targetcount - m_successcount));//未完成数
        double hourCount = ui->listWidget->count() / (m_elapsed_second / 3600.00);
        qInfo() << "hourCount:" << hourCount << m_elapsed_second << ui->listWidget->count();
        ui->label_48->setText(QString::number(hourCount, 'f', 0)+" units/hour");//平均速度
        ui->pushButton_4->setEnabled(true);
    }
}

void MainWindow::addProgressList(Controller *controller)
{
    int rowCount = ui->tableWidget_2->rowCount();
    ui->tableWidget_2->insertRow(rowCount);//添加到正在升级列表
    ui->tableWidget_2->setItem(rowCount, 0, new QTableWidgetItem(controller->address()));
    controller->ConnectDevice(setup->m_startTimeout);
    controller_progress_list << controller;
    m_processingcount ++;
}

//OTA开始升级
void MainWindow::on_pushButton_4_clicked()
{
#ifndef SAMPLE
//    agent->startScanDevice(10000, (QStringList()<<"3D:D8"<<"3F:E1"<<"57:E7"<<"6E:E9"<<"40:E8"));
    if (0 == setup->m_total_file_size) {
        QMessageBox::information(this, "Tips", QString::fromLocal8Bit("请检查升级包!"));
        return ;
    }
    if (ui->label_20->text().isEmpty()) {
        if (QMessageBox::question(this, "Ask", QString::fromLocal8Bit("版本号为空，确定要升级吗?"))
                == QMessageBox::No) {
            return ;
        }
    }
    if (!ui->lineEdit_5->text().isEmpty() &&
            ui->lineEdit_4->text().toInt()) {
        m_targetcount = ui->lineEdit_4->text().toInt();
    } else {
        m_targetcount = setup->m_address_list.size();
    }
    qInfo() << "Target count:" << m_targetcount;
    if (m_targetcount < 1)
    {
        QMessageBox::information(this, "Tips", QString::fromLocal8Bit("请确认目标数"));
        return ;
    }

    //如果目标数小于最大队列数
    if (m_targetcount < setup->m_queuemax)
    {
        setup->m_queuemax = m_targetcount;
        qInfo() << "reset queue max:" << setup->m_queuemax;
    }
    m_successcount = 0;
    m_failcount = 0;

    agent->setMatchStr(ui->lineEdit_5->text());
    ui->label_32->setText(QString::number(m_targetcount));
    ui->label_40->setText(QString::number(m_targetcount));//目标总数
    ui->tabWidget->setCurrentIndex(4);
    m_timer->start(1000);
    ui->pushButton_4->setEnabled(false);

    agent->startScan();
#else
    device->startDeviceDiscovery();
#endif
}

void MainWindow::on_pushButton_3_clicked()
{

}

//OTA结束
void MainWindow::on_pushButton_6_clicked()
{
#ifndef SAMPLE
    m_elapsed_second = 0;
    ui->tableWidget_2->clearContents();
    ui->tableWidget_2->setRowCount(0);
    ui->listWidget->clear();
    m_timer->stop();
    agent->cancelScan();
    qDeleteAll(controller_list);
    controller_list.clear();
    ui->pushButton_4->setEnabled(true);
#else
    device->disconnectFromDevice();
#endif
}


//账号登录
void MainWindow::on_pushButton_clicked()
{
    if (ui->lineEdit->text().isEmpty() || ui->lineEdit_2->text().isEmpty()) {
        return ;
    }
    int ret = https->login(ui->lineEdit->text(), ui->lineEdit_2->text(), ui->lineEdit_3->text());
    if (ret < 0) {
        QMessageBox::information(this, "Tips", QString::fromLocal8Bit("登录失败"));
        refreshVCode();
        return ;
    }
    QList<QStringList> stringList;
    ret = https->upgradePackageList(stringList);
    if (ret < 0) {
        QMessageBox::question(this, "Ask", QString::fromLocal8Bit("获取列表失败"),
                              QMessageBox::Retry | QMessageBox::Cancel);
        return;
    }

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    qDebug() << "stringList:" << stringList.size();
    ui->tabWidget->setCurrentIndex(1);
    for (auto &list : stringList) {
        int column = 1;
        int rowCount = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(rowCount);
        ui->tableWidget->setItem(rowCount, 0, new QTableWidgetItem(list.value(0)));
        list.removeFirst();
        for (auto &string : list) {
            qDebug() << rowCount << column << string;
            ui->tableWidget->setItem(rowCount, column ++, new QTableWidgetItem(string));
        }
        qDebug() << "OTAID:" << ui->tableWidget->item(rowCount, 0)->text();
    }
}


void MainWindow::on_pushButton_2_clicked()
{
    int currentRow = ui->tableWidget->currentRow();
    qDebug() << currentRow;
    if (currentRow >= 0) {
        ui->tabWidget->setCurrentIndex(3);
        if (nullptr == ui->tableWidget->item(currentRow, 0)
                || nullptr == ui->tableWidget->item(currentRow, 2)) {
            return ;
        }
        int custOtaId = ui->tableWidget->item(currentRow, 0)->text().toInt();
        QString version = ui->tableWidget->item(currentRow, 2)->text();

        qDebug() << "custOtaId:" << custOtaId << "version:" << version;
        QString dirname;
        if (https->downloadPackage(custOtaId, dirname) < 0) {
            qWarning() << "download fail:";
            QMessageBox::information(this, "Tips", QString::fromLocal8Bit("下载失败"));
            return ;
        }
        qDebug() << "download successfully:" << dirname;
        GetDirectoryFile(dirname);
        ui->label_19->setText(dirname);
        ui->label_20->setText(version);
        ui->label_31->setText(version);
        ui->label_38->setText(version);
        setup->m_version = version.toUtf8();
        setup->m_version.resize(12);
    }
}

