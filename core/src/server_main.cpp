#include <unistd.h>

#include "../include/virtual_gamepad.h"
#include "../include/udp_receiver.h"
#include "../include/controller_engine.h"
#include "../include/protocol.h"
#include "../include/discovery_service.h"
#include "../include/server_main.h"

#include "QThread"

void Server::startServer() {
    running = true;
    VirtualGamepad gp;
    if (!gp.init()) {
        emit sendData1("Gamepad init failed");
        return ;
    }

    UdpReceiver udp;
    if (!udp.start(9000)) {
        emit sendData1("UDP bind failed\n");
        return ;
    }

    ControllerEngine engine(&gp);

    lj::DiscoveryService discovery(9002, [&] {
        lj::ServerStatus st;
        st.paired_locked = engine.isPairedLocked();

        st.control_port = 9000;
        st.controller_proto_ver = 1;
        st.flags = 0;
        st.name = "linux-joystick";
        st.server_id = 0;
        return st;
    });

    if (!discovery.start()) {
        emit sendData1("Discovery bind failed (UDP 9002)\n");
        return ;
    }

    ControllerPacketV1 pkt{};
    sockaddr_in sender{};

    emit sendData1("GAMEPAD INITIALIZATION SUCCESS");

    while (running) {
        int n = udp.receive(&pkt, sizeof(pkt), &sender);
        if (n > 0) {
            engine.processPacket(&pkt, n, sender);
        }
    }
    discovery.stop();
    emit sendData1("Server Stopped");
    return ;
}

void Server::stopServer(){
    running = false;
}