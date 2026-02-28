#include "discovery_service.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

DiscoveryService::DiscoveryService()
    : sockfd_(-1), discovery_port_(0), control_port_(0), vibration_port_(0), running_(false) {}

DiscoveryService::~DiscoveryService() { stop(); }

bool DiscoveryService::start(int discovery_port, int control_port, int vibration_tcp_port) {
    discovery_port_ = discovery_port;
    control_port_ = control_port;
    vibration_port_ = vibration_tcp_port;

    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(discovery_port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        last_error_ = std::strerror(errno);
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    running_ = true;
    worker_ = std::thread(&DiscoveryService::runLoop, this);
    return true;
}

void DiscoveryService::runLoop() {
    constexpr char query[] = "WHO_IS_CONTROLLER";

    while (running_) {
        char buffer[512] = {};
        sockaddr_in client_addr {};
        socklen_t client_len = sizeof(client_addr);

        int n = recvfrom(sockfd_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&client_addr),
                         &client_len);

        if (n <= 0) {
            continue;
        }

        std::string msg(buffer, n);
        if (msg == query) {
            std::string reply = "CONTROLLER_AVAILABLE;udp=" + std::to_string(control_port_) +
                                ";tcp=" + std::to_string(vibration_port_);
            sendto(sockfd_, reply.c_str(), reply.size(), 0, reinterpret_cast<sockaddr*>(&client_addr),
                   client_len);
        }
    }
}

void DiscoveryService::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    shutdown(sockfd_, SHUT_RDWR);

    if (worker_.joinable()) {
        worker_.join();
    }

    close(sockfd_);
    sockfd_ = -1;
}

std::string DiscoveryService::lastError() const { return last_error_; }
