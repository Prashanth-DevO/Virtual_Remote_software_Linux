#pragma once
#include <netinet/in.h>

class UdpReceiver {
public:
    bool start(int port);
    int receive(void* buffer, int size, sockaddr_in* outSender);
};