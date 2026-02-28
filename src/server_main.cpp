#include "virtual_gamepad.h"
#include "udp_receiver.h"
#include <iostream>
#include "gamepad_mapping.h"

int main() {

    VirtualGamepad gamepad;

    if(!gamepad.init()) {
        std::cout << "Gamepad init failed\n";
        return -1;
    }

    UdpReceiver udp;

    if(!udp.start(9000)) {
        std::cout << "UDP failed\n";
        return -1;
    }

    std::cout << "Server Running...\n";

    char buffer[1024];

    while(true) {
        int len = udp.receive(buffer, sizeof(buffer));

        if(len > 0) {
            gamepad.sendButton(GP_A, 1);
            gamepad.sync();

            gamepad.sendButton(GP_A, 0);
            gamepad.sync();
        }
    }
}