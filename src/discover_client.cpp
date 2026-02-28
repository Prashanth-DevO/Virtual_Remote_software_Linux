#include "discovery_protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

using namespace lj;

static uint32_t randNonce() {
    static std::mt19937 rng{std::random_device{}()};
    return (uint32_t)rng();
}

int main() {
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    int one = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));

    // bind ephemeral
    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(0);
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(sock, (sockaddr*)&bindAddr, sizeof(bindAddr)) < 0) {
        std::cerr << "bind failed\n";
        return 1;
    }

    DiscoverReqV1 req{};
    req.magic = kDiscoveryMagic;
    req.version = kDiscoveryVersion;
    req.msg_type = (uint8_t)DiscMsgType::DiscoverReq;
    req.reserved = 0;
    req.nonce = randNonce();

    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(9002);
    dst.sin_addr.s_addr = inet_addr("255.255.255.255");

    if (::sendto(sock, &req, sizeof(req), 0, (sockaddr*)&dst, sizeof(dst)) < 0) {
        std::cerr << "sendto failed\n";
        return 1;
    }

    std::cout << "Sent discovery nonce=" << req.nonce << "\n";
    std::cout << "Waiting 500ms for responses...\n";

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    std::vector<uint8_t> buf(2048);

    while (std::chrono::steady_clock::now() < deadline) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000; // 100ms

        int r = ::select(sock + 1, &fds, nullptr, nullptr, &tv);
        if (r <= 0) continue;

        sockaddr_in src{};
        socklen_t slen = sizeof(src);
        ssize_t n = ::recvfrom(sock, buf.data(), buf.size(), 0, (sockaddr*)&src, &slen);
        if (n < (ssize_t)sizeof(DiscoverRespV1)) continue;

        DiscoverRespV1 resp{};
        std::memcpy(&resp, buf.data(), sizeof(resp));

        if (resp.magic != kDiscoveryMagic) continue;
        if (resp.version != kDiscoveryVersion) continue;
        if (resp.msg_type != (uint8_t)DiscMsgType::DiscoverResp) continue;
        if (resp.nonce != req.nonce) continue;

        size_t expected = sizeof(DiscoverRespV1) + resp.name_len;
        if ((size_t)n < expected) continue;

        std::string name((char*)buf.data() + sizeof(DiscoverRespV1),
                         (char*)buf.data() + sizeof(DiscoverRespV1) + resp.name_len);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src.sin_addr, ip, sizeof(ip));

        std::cout << "Found server: " << ip
                  << " name=" << name
                  << " control=" << resp.control_port
                  << " feedback=" << resp.feedback_port
                  << " proto=" << resp.proto_ver
                  << " flags=0x" << std::hex << resp.flags << std::dec
                  << " server_id=" << resp.server_id
                  << "\n";
    }

    ::close(sock);
    return 0;
}