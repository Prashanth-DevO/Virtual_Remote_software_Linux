#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QThread"

#include "../core/include/server_main.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void ipUpdate(uint32_t ip);
    void logMessage(QString &msg);
    void updateTimerDisplay();

private slots:
    void on_get_IP_clicked();

    void on_systemStart_clicked();

    void on_systemStop_clicked();

    void on_clearLogBtn_clicked();

private:
    Ui::MainWindow *ui;
    Server *server = nullptr;
    QThread *thread  = nullptr;
    QTimer *timer;
    int elapsedSeconds =0 ;
};
#endif // MAINWINDOW_H
