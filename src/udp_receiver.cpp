#include "udp_receiver.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

UdpReceiver::UdpReceiver() : sockfd_(-1) {}

UdpReceiver::~UdpReceiver() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool UdpReceiver::start(int port) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        last_error_ = std::strerror(errno);
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    return true;
}

int UdpReceiver::receive(char* buffer, int size, sockaddr_in* sender) {
    sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    int received = recvfrom(sockfd_, buffer, size, 0, reinterpret_cast<sockaddr*>(&from), &from_len);

    if (received < 0) {
        last_error_ = std::strerror(errno);
        return received;
    }

    if (sender != nullptr) {
        *sender = from;
    }

    return received;
}

std::string UdpReceiver::lastError() const {
    return last_error_;
}