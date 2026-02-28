#include "protocol.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/time.h>

static uint64_t nowUs(){
    timeval tv{}; gettimeofday(&tv, nullptr);
    return uint64_t(tv.tv_sec) * 1000000ULL + uint64_t(tv.tv_usec);
}

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { std::cout << "socket fail\n"; return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    ControllerPacketV1 p{};
    p.magic = UNIO_MAGIC;
    p.version = UNIO_V1;
    p.size = sizeof(ControllerPacketV1);
    p.seq = 1;
    p.ts_us = nowUs();

    // TEST 1: A pressed + LX max right
    p.lx = 32767;
    p.ly = 0;
    p.rx = 0;
    p.ry = 0;
    p.l2 = 0;
    p.r2 = 0;
    p.dpad_x = 0;
    p.dpad_y = 0;
    p.buttons = (1u << BTNBIT_A);

    sendto(s, &p, sizeof(p), 0, (sockaddr*)&addr, sizeof(addr));

    // TEST 2: release A + center stick (after 300ms)
    usleep(300000);
    p.seq++;
    p.ts_us = nowUs();
    p.lx = 0;
    p.buttons = 0;
    sendto(s, &p, sizeof(p), 0, (sockaddr*)&addr, sizeof(addr));

    close(s);
    std::cout << "Sent test packets\n";
}