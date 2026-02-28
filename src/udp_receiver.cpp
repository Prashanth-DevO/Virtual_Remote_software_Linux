#include "udp_receiver.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int sockfd;

bool UdpReceiver::start(int port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0)
        return false;

    return true;
}

int UdpReceiver::receive(char* buffer, int size) {
    return recv(sockfd, buffer, size, 0);
}