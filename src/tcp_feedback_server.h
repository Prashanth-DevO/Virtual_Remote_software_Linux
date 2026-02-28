#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class TcpFeedbackServer {
public:
    TcpFeedbackServer();
    ~TcpFeedbackServer();

    bool start(int port);
    void stop();

    bool sendVibration(int duration_ms, int strength);
    bool hasClient() const;
    std::string lastError() const;

private:
    void acceptLoop();

    int listen_fd_;
    int client_fd_;
    std::atomic<bool> running_;
    std::thread worker_;
    mutable std::mutex client_mutex_;
    std::string last_error_;
};
