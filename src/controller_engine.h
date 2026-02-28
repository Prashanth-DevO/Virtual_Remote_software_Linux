#pragma once
#include "virtual_gamepad.h"
#include <netinet/in.h>
#include <cstdint>
#include <atomic>

class ControllerEngine {
public:
    explicit ControllerEngine(VirtualGamepad* gp);

    // returns true if packet applied
    bool processPacket(const void* data, int len, const sockaddr_in& sender);

    bool isPairedLocked() const {
        return locked_.load(std::memory_order_relaxed);
    }

private:
    VirtualGamepad* gamepad;

    // pairing + watchdog
    std::atomic<bool> locked_{false};   // was bool locked
    uint32_t authorized_ip = 0;         // sender.sin_addr.s_addr (written in UDP thread only)
    uint64_t lastPacketUs = 0;          // written in UDP thread only

    void neutral();
    static uint64_t nowUs();
};