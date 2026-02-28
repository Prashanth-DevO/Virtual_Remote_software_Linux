#include "tcp_feedback_server.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

TcpFeedbackServer::TcpFeedbackServer() : listen_fd_(-1), client_fd_(-1), running_(false) {}

TcpFeedbackServer::~TcpFeedbackServer() { stop(); }

bool TcpFeedbackServer::start(int port) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    int reuse = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || listen(listen_fd_, 1) < 0) {
        last_error_ = std::strerror(errno);
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    running_ = true;
    worker_ = std::thread(&TcpFeedbackServer::acceptLoop, this);
    return true;
}

void TcpFeedbackServer::acceptLoop() {
    while (running_) {
        sockaddr_in client_addr {};
        socklen_t len = sizeof(client_addr);
        int fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (fd < 0) {
            continue;
        }

        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_fd_ >= 0) {
            close(client_fd_);
        }
        client_fd_ = fd;
    }
}

bool TcpFeedbackServer::sendVibration(int duration_ms, int strength) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_fd_ < 0) {
        return false;
    }

    if (duration_ms < 0) {
        duration_ms = 0;
    }
    if (strength < 0) {
        strength = 0;
    }
    if (strength > 255) {
        strength = 255;
    }

    std::string msg = "VIBRATE:" + std::to_string(duration_ms) + ":" + std::to_string(strength) + "\n";

    ssize_t sent = send(client_fd_, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    if (sent < 0) {
        last_error_ = std::strerror(errno);
        close(client_fd_);
        client_fd_ = -1;
        return false;
    }

    return true;
}

bool TcpFeedbackServer::hasClient() const {
    std::lock_guard<std::mutex> lock(client_mutex_);
    return client_fd_ >= 0;
}

void TcpFeedbackServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    shutdown(listen_fd_, SHUT_RDWR);

    if (worker_.joinable()) {
        worker_.join();
    }

    {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_fd_ >= 0) {
            close(client_fd_);
            client_fd_ = -1;
        }
    }

    close(listen_fd_);
    listen_fd_ = -1;
}

std::string TcpFeedbackServer::lastError() const { return last_error_; }
