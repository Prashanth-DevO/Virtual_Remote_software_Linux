#ifndef SERVER_MAIN_CPP
#define SERVER_MAIN_CPP

#include "QString"
#include <QObject>
#include <atomic>

class Server : public QObject {
    Q_OBJECT

    public:
    std::atomic<bool> running{false};
    public slots:
    void startServer();
    void stopServer();
    signals:
    void sendData1(QString);
    void sendIP(uint32_t);
};

#endif // SERVER_MAIN_CPP
