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

private slots:
    void on_get_IP_clicked();

    void on_systemStart_clicked();

    void on_systemStop_clicked();

private:
    Ui::MainWindow *ui;
    Server *server = nullptr;
    QThread *thread  = nullptr;
};
#endif // MAINWINDOW_H
