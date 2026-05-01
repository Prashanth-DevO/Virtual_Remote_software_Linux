#include <unistd.h>

#include "../include/virtual_gamepad.h"
#include "../include/udp_receiver.h"
#include "../include/controller_engine.h"
#include "../include/protocol.h"
#include "../include/discovery_service.h"
#include "../include/server_main.h"

#include <QThread>

void Server::startServer() {
    running.store(true, std::memory_order_relaxed);
    VirtualGamepad gp;
    if (!gp.init()) {
        emit sendData1("Gamepad init failed\n try to change the file permession of /dev/uinput to 666\nsudo chmod 666 /dev/uinput");
        return ;
    }

    UdpReceiver udp;
    if (!udp.start(9000)) {
        emit sendData1("UDP bind failed\n");
        return ;
    }

    ControllerEngine engine(&gp);
    connect(&engine, &ControllerEngine::sendData1, this, &Server::sendData1);

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
    connect(&discovery, &lj::DiscoveryService::sendData1 , this ,&Server::sendData1);

    if (!discovery.start()) {
        emit sendData1("Discovery bind failed (UDP 9002)\n");
        return ;
    }

    ControllerPacketV1 pkt{};
    sockaddr_in sender{};

    emit sendData1("GAMEPAD INITIALIZATION SUCCESS");

    while (running.load(std::memory_order_relaxed)) {
        int n = udp.receive(&pkt, sizeof(pkt), &sender);
        if (n > 0) {
            engine.processPacket(&pkt, n, sender);
        } else {
            QThread::msleep(10);
        }
    }
    udp.stop();
    discovery.stop();
    emit sendData1("Server Stopped");
    return ;
}

void Server::stopServer(){
    running.store(false, std::memory_order_relaxed);
}