#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace lj {

struct ServerStatus {
    bool paired_locked = false;
    uint16_t control_port = 9000;
    uint16_t controller_proto_ver = 1;
    uint32_t flags = 0;
    uint64_t server_id = 0;
    std::string name;
};

class DiscoveryService {
public:
    using StatusFn = std::function<ServerStatus()>;

    DiscoveryService(uint16_t port, StatusFn status_fn);
    ~DiscoveryService();

    bool start();
    void stop();

private:
    void runLoop();
    bool setupSocket();

    uint16_t port_;
    StatusFn status_fn_;

    int sock_ = -1;
    std::atomic<bool> running_{false};
    std::thread th_;
};

} // namespace lj
