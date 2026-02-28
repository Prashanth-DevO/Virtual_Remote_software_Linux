#pragma once

#include <netinet/in.h>
#include <string>

class UdpReceiver {
public:
    UdpReceiver();
    ~UdpReceiver();

    bool start(int port);
    int receive(char* buffer, int size, sockaddr_in* sender = nullptr);
    std::string lastError() const;

private:
    int sockfd_;
    std::string last_error_;
};