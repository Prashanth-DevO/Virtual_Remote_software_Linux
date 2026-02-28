#pragma once

class UdpReceiver {
public:
    bool start(int port);
    int receive(char* buffer, int size);
};