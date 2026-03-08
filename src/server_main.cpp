#include <iostream>
#include <unistd.h>

#include "virtual_gamepad.h"
#include "udp_receiver.h"
#include "controller_engine.h"
#include "protocol.h"

#include "discovery_service.h"   
#include "discovery_protocol.h"  

#include "tcp_feedback_server.h"

int main() {
    TcpFeedbackServer feedback_server;
    if(!feedback_server.start(9001)) {
        std::cout << "Feedback server failed to start: " << feedback_server.lastError() << "\n";
        return 1;
    }

    VirtualGamepad gp(&feedback_server);
    if (!gp.init()) {
        std::cout << "Gamepad init failed\n";
        return 1;
    }

    UdpReceiver udp;
    if (!udp.start(9000)) {
        std::cout << "UDP bind failed\n";
        return 1;
    }

    ControllerEngine engine(&gp);

    lj::DiscoveryService discovery(9002, [&] {
        lj::ServerStatus st;
        st.paired_locked = engine.isPairedLocked();

        st.control_port = 9000;
        st.feedback_port = 9001;
        st.controller_proto_ver = 1;
        st.flags = 0;
        st.name = "linux-joystick";
        st.server_id = 0;
        return st;
    });

    if (!discovery.start()) {
        std::cout << "Discovery bind failed (UDP 9002)\n";
        return 1;
    }

    ControllerPacketV1 pkt{};
    sockaddr_in sender{};

    std::cout << "Server running:\n";
    std::cout << "  UDP control   : 9000\n";
    std::cout << "  UDP discovery : 9002\n";
    std::cout << "  TCP feedback  : 9001\n";
    while (true) {
        int n = udp.receive(&pkt, sizeof(pkt), &sender);
        if (n > 0) {
            engine.processPacket(&pkt, n, sender);
        }
        gp.processForceFeedback();
    }

    discovery.stop();
    return 0;
}
