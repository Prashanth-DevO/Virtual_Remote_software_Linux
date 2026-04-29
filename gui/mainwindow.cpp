#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "string"
#include "QString"
#include "../core/include/utils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_get_IP_clicked()
{
    std::string str = getLocalIP();
    QString strIP = QString::fromStdString(str);
    ui->SystemIPLabel->setText(strIP);
}

