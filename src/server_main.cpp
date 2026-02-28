#include <arpa/inet.h>

#include <iostream>
#include <string>

#include "controller_engine.h"
#include "discovery_service.h"
#include "tcp_feedback_server.h"
#include "udp_receiver.h"
#include "virtual_gamepad.h"

namespace {
constexpr int kControlPort = 9000;
constexpr int kDiscoveryPort = 9002;
constexpr int kVibrationTcpPort = 9001;

bool tryForwardVibration(TcpFeedbackServer& tcp_feedback, const std::string& msg) {
    if (msg.rfind("VIBRATE:", 0) != 0) {
        return false;
    }

    size_t first_colon = msg.find(':');
    size_t second_colon = msg.find(':', first_colon + 1);
    if (second_colon == std::string::npos) {
        return false;
    }

    int duration = std::atoi(msg.substr(first_colon + 1, second_colon - first_colon - 1).c_str());
    int strength = std::atoi(msg.substr(second_colon + 1).c_str());

    return tcp_feedback.sendVibration(duration, strength);
}
}  // namespace

int main() {
    VirtualGamepad gamepad;
    if (!gamepad.init()) {
        std::cerr << "Gamepad init failed: " << gamepad.lastError() << "\n";
        return 1;
    }

    UdpReceiver udp;
    if (!udp.start(kControlPort)) {
        std::cerr << "UDP receiver start failed on " << kControlPort << ": " << udp.lastError() << "\n";
        return 1;
    }

    DiscoveryService discovery;
    if (!discovery.start(kDiscoveryPort, kControlPort, kVibrationTcpPort)) {
        std::cerr << "Discovery service failed on " << kDiscoveryPort << ": " << discovery.lastError() << "\n";
        return 1;
    }

    TcpFeedbackServer tcp_feedback;
    if (!tcp_feedback.start(kVibrationTcpPort)) {
        std::cerr << "TCP feedback server failed on " << kVibrationTcpPort << ": " << tcp_feedback.lastError()
                  << "\n";
        return 1;
    }

    ControllerEngine controller(&gamepad);

    std::cout << "Linux remote server running. UDP control=" << kControlPort
              << ", UDP discovery=" << kDiscoveryPort << ", TCP vibration=" << kVibrationTcpPort << "\n";

    while (true) {
        char buffer[1024] = {};
        sockaddr_in sender {};
        int len = udp.receive(buffer, sizeof(buffer), &sender);
        if (len <= 0) {
            continue;
        }

        std::string msg(buffer, len);
        if (controller.process(buffer, len)) {
            continue;
        }

        if (tryForwardVibration(tcp_feedback, msg)) {
            continue;
        }

        if (msg == "PING") {
            std::cout << "PING from " << inet_ntoa(sender.sin_addr) << "\n";
        }
    }
}