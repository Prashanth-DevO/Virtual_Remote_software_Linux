#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <QString>
#include <QDateTime>
#include "../core/include/utils.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    if (thread) {
        if (server) {
            server->stopServer();
        }
        thread->quit();
    }
    delete ui;
}

void MainWindow::on_get_IP_clicked()
{
    std::string str = getLocalIP();
    QString strIP = QString::fromStdString(str);
    ui->SystemIPLabel->setText(strIP);
}


void MainWindow::on_systemStart_clicked()
{
    if(thread && thread->isRunning()) return ;
    thread = new QThread();
    server = new Server();

    server->moveToThread(thread);

    connect(thread, &QThread::started, server, &Server::startServer);

    connect(server, &Server::sendData1, this, [this](QString msg){
        logMessage(msg);
    });

    connect(server, &Server::sendIP, this ,[this](uint32_t ip){
        ipUpdate(ip);
    });

    connect(thread, &QThread::finished, server, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this]() {
        server = nullptr;
        thread = nullptr;
    });

    thread->start();
}

void MainWindow::on_systemStop_clicked()
{
    if(!server || !thread) return ;

    server->stopServer();
    thread->quit();
    ui->ConnectedIPLabel->setText("----");
    ui->connectionStatusLabel->setText("Unpaired");
}


void MainWindow::on_clearLogBtn_clicked()
{
    ui->logs_text->clear();
}

void MainWindow::logMessage(QString &msg){
    QString log = QDateTime::currentDateTime()
    .toString("[HH:mm:ss] ") + msg;
    ui->logs_text->appendPlainText(log);
}

void MainWindow::ipUpdate(uint32_t ip){
    QString ipString = QString("%1.%2.%3.%4")
    .arg((ip >> 24) & 0xFF)
        .arg((ip >> 16) & 0xFF)
        .arg((ip >> 8) & 0xFF)
        .arg(ip & 0xFF);
    ui->ConnectedIPLabel->setText(ipString);
    ui->connectionStatusLabel->setText("Paired Successfully");
}
