#include "controller_engine.h"
#include "protocol.h"
#include "gamepad_mapping.h"

#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/time.h>

ControllerEngine::ControllerEngine(VirtualGamepad* gp) : gamepad(gp) {}

uint64_t ControllerEngine::nowUs() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return uint64_t(tv.tv_sec) * 1000000ULL + uint64_t(tv.tv_usec);
}

void ControllerEngine::neutral() {
    gamepad->sendAxis(GP_LX, 0);
    gamepad->sendAxis(GP_LY, 0);
    gamepad->sendAxis(GP_RX, 0);
    gamepad->sendAxis(GP_RY, 0);

    gamepad->sendAxis(GP_L2, 0);
    gamepad->sendAxis(GP_R2, 0);

    gamepad->sendAxis(GP_DPAD_X, 0);
    gamepad->sendAxis(GP_DPAD_Y, 0);

    gamepad->sendButton(GP_A, 0);
    gamepad->sendButton(GP_B, 0);
    gamepad->sendButton(GP_X, 0);
    gamepad->sendButton(GP_Y, 0);
    gamepad->sendButton(GP_L1, 0);
    gamepad->sendButton(GP_R1, 0);
    gamepad->sendButton(GP_L3, 0);
    gamepad->sendButton(GP_R3, 0);
    gamepad->sendButton(GP_SELECT, 0);
    gamepad->sendButton(GP_START, 0);

    gamepad->sync();
}

bool ControllerEngine::processPacket(const void* data, int len, const sockaddr_in& sender) {
    const uint64_t now = nowUs();
     
    // watchdog: neutral if silent, unlock after longer silence
    if (lastPacketUs != 0) {
        const uint64_t diffMs = (now - lastPacketUs) / 1000ULL;
        if (diffMs > 200) neutral();
        if (diffMs > 2000){ 
            authorized_ip = 0;
            locked_.store(false, std::memory_order_relaxed);
            neutral();
        }
    }

    if (len < (int)sizeof(ControllerPacketV1)) return false;

    ControllerPacketV1 pkt{};
    std::memcpy(&pkt, data, sizeof(pkt));

    if (pkt.magic != UNIO_MAGIC || pkt.version != UNIO_V1 || pkt.size != sizeof(ControllerPacketV1)) {
        return false;
    }

    // pairing lock
    uint32_t ip = sender.sin_addr.s_addr;

    if (!locked_.load(std::memory_order_relaxed)) {
        authorized_ip = ip;
        locked_.store(true, std::memory_order_relaxed);
        std::cerr << "[lock] locking to ip=" << ip << "\n";
    }

    if (locked_.load(std::memory_order_relaxed) && ip != authorized_ip) {
        // ignore packets from other IPs while locked
        if (pkt.magic != UNIO_MAGIC || pkt.version != UNIO_V1 || pkt.size != sizeof(ControllerPacketV1)) {
            std::cerr << "[proto] reject: magic=" << std::hex << pkt.magic << std::dec
                      << " ver=" << pkt.version
                      << " pkt.size=" << pkt.size
                      << " expected=" << sizeof(ControllerPacketV1) << "\n";
            return false;
        }
        return false;
    }
    lastPacketUs = now;

    // axes
    gamepad->sendAxis(GP_LX, pkt.lx);
    gamepad->sendAxis(GP_LY, pkt.ly);
    gamepad->sendAxis(GP_RX, pkt.rx);
    gamepad->sendAxis(GP_RY, pkt.ry);

    gamepad->sendAxis(GP_L2, pkt.l2);
    gamepad->sendAxis(GP_R2, pkt.r2);

    gamepad->sendAxis(GP_DPAD_X, pkt.dpad_x);
    gamepad->sendAxis(GP_DPAD_Y, pkt.dpad_y);

    // buttons
    auto bit = [&](uint32_t b)->int { return (pkt.buttons >> b) & 1U; };

    gamepad->sendButton(GP_A,      bit(BTNBIT_A));
    gamepad->sendButton(GP_B,      bit(BTNBIT_B));
    gamepad->sendButton(GP_X,      bit(BTNBIT_X));
    gamepad->sendButton(GP_Y,      bit(BTNBIT_Y));
    gamepad->sendButton(GP_L1,     bit(BTNBIT_L1));
    gamepad->sendButton(GP_R1,     bit(BTNBIT_R1));
    gamepad->sendButton(GP_L3,     bit(BTNBIT_L3));
    gamepad->sendButton(GP_R3,     bit(BTNBIT_R3));
    gamepad->sendButton(GP_SELECT, bit(BTNBIT_SELECT));
    gamepad->sendButton(GP_START,  bit(BTNBIT_START));
    // HOME optional if you mapped GP_HOME: gamepad->sendButton(GP_HOME, bit(BTNBIT_HOME));

    gamepad->sync();
    if (pkt.magic != UNIO_MAGIC || pkt.version != UNIO_V1 || pkt.size != sizeof(ControllerPacketV1)) {
        std::cerr << "[proto] reject: magic=" << std::hex << pkt.magic << std::dec
                  << " ver=" << pkt.version
                  << " pkt.size=" << pkt.size
                  << " expected=" << sizeof(ControllerPacketV1) << "\n";
        return false;
    }
    
    return true;
}