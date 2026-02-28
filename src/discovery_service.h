#pragma once

#include <atomic>
#include <string>
#include <thread>

class DiscoveryService {
public:
    DiscoveryService();
    ~DiscoveryService();

    bool start(int discovery_port, int control_port, int vibration_tcp_port);
    void stop();

    std::string lastError() const;

private:
    void runLoop();

    int sockfd_;
    int discovery_port_;
    int control_port_;
    int vibration_port_;
    std::atomic<bool> running_;
    std::thread worker_;
    std::string last_error_;
};
