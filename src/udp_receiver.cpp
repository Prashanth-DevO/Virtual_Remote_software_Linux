#include "../include/udp_receiver.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

static int sockfd = -1;

bool UdpReceiver::start(int port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return false;

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
    return true;
}

int UdpReceiver::receive(void* buffer, int size, sockaddr_in* outSender) {
    socklen_t len = sizeof(sockaddr_in);
    std::memset(outSender, 0, sizeof(sockaddr_in));
    return recvfrom(sockfd, buffer, size, 0, (sockaddr*)outSender, &len);
}