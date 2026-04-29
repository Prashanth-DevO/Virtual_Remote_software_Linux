#ifndef SERVER_MAIN_CPP
#define SERVER_MAIN_CPP

#include "QString"
#include <QObject>

class Server : public QObject {
    Q_OBJECT

    public:
    bool running = false;
    public slots:
    void startServer();
    void stopServer();
    signals:
    void sendData1(QString);
};

#endif // SERVER_MAIN_CPP
