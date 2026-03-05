#include "discovery_service.h"
#include "discovery_protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace lj {

// -------------------- Stable server_id: hash(/etc/machine-id) --------------------

static uint64_t fnv1a64(const uint8_t* data, size_t len) {
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)data[i];
        h *= 1099511628211ull;
    }
    return h;
}

static uint64_t stableServerId() {
    // Cache once per process
    static uint64_t cached = 0;
    if (cached != 0) return cached;

    std::ifstream f("/etc/machine-id");
    if (f) {
        std::string id;
        std::getline(f, id);

        // Remove whitespace/newlines just in case
        id.erase(std::remove_if(id.begin(), id.end(), ::isspace), id.end());

        if (!id.empty()) {
            cached = fnv1a64(reinterpret_cast<const uint8_t*>(id.data()), id.size());
            if (cached != 0) return cached;
        }
    }

    // Fallback if machine-id missing (rare)
    const char* s = "linux-joystick-fallback";
    cached = fnv1a64(reinterpret_cast<const uint8_t*>(s), std::strlen(s));
    return cached;
}

// -------------------- DiscoveryService --------------------

DiscoveryService::DiscoveryService(uint16_t port, StatusFn status_fn)
    : port_(port), status_fn_(std::move(status_fn)) {}

DiscoveryService::~DiscoveryService() {
    stop();
}

bool DiscoveryService::setupSocket() {
    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        std::cerr << "[discovery] socket() failed: " << strerror(errno) << "\n";
        return false;
    }

    int one = 1;
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    ::setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[discovery] bind() failed on port " << port_
                  << ": " << strerror(errno) << "\n";
        ::close(sock_);
        sock_ = -1;
        return false;
    }

    return true;
}

bool DiscoveryService::start() {
    if (running_.load()) return true;
    if (!setupSocket()) return false;

    running_.store(true);
    th_ = std::thread(&DiscoveryService::runLoop, this);
    return true;
}

void DiscoveryService::stop() {
    if (!running_.exchange(false)) return;

    // Closing socket unblocks recvfrom
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }

    if (th_.joinable()) th_.join();
}

void DiscoveryService::runLoop() {
    std::vector<uint8_t> buf(1024);

    // Simple per-IP rate limit: respond at most once per 150ms per source IP
    using Clock = std::chrono::steady_clock;
    std::unordered_map<uint32_t, Clock::time_point> lastReply;
    lastReply.reserve(64);

    const auto minInterval = std::chrono::milliseconds(150);

    while (running_.load()) {
        sockaddr_in src{};
        socklen_t slen = sizeof(src);
        ssize_t n = ::recvfrom(sock_, buf.data(), buf.size(), 0, (sockaddr*)&src, &slen);

        if (n < 0) {
            if (!running_.load()) break;
            continue;
        }

        // Strict minimum size check
        if ((size_t)n < sizeof(DiscoverReqV1)) continue;

        DiscoverReqV1 req{};
        std::memcpy(&req, buf.data(), sizeof(req));

        // Validate request
        if (req.magic != kDiscoveryMagic) continue;
        if (req.version != kDiscoveryVersion) continue;
        if (req.msg_type != (uint8_t)DiscMsgType::DiscoverReq) continue;

        // Rate limit by source IPv4
        uint32_t ipKey = src.sin_addr.s_addr; // network order is fine as a key
        auto now = Clock::now();
        auto it = lastReply.find(ipKey);
        if (it != lastReply.end() && (now - it->second) < minInterval) {
            continue;
        }
        lastReply[ipKey] = now;

        // Build response from status provider
        ServerStatus st = status_fn_ ? status_fn_() : ServerStatus{};

        // Stable ID unless caller explicitly overrides
        if (st.server_id == 0) st.server_id = stableServerId();

        if (st.name.empty()) st.name = "LINUX_VIRTUAL_BOX 🎮";

        uint32_t flags = st.flags;
        if (st.paired_locked) flags |= kFlagPairedLocked;

        std::string name = st.name;
        if (name.size() > kMaxNameLen) name.resize(kMaxNameLen);

        DiscoverRespV1 resp{};
        resp.magic = kDiscoveryMagic;
        resp.version = kDiscoveryVersion;
        resp.msg_type = (uint8_t)DiscMsgType::DiscoverResp;
        resp.reserved = 0;
        resp.nonce = req.nonce;
        resp.server_id = st.server_id;
        resp.control_port = st.control_port;
        resp.feedback_port = st.feedback_port;
        resp.proto_ver = st.controller_proto_ver;
        resp.name_len = (uint16_t)name.size();
        resp.flags = flags;

        std::vector<uint8_t> out(sizeof(DiscoverRespV1) + name.size());
        std::memcpy(out.data(), &resp, sizeof(resp));
        if (!name.empty()) {
            std::memcpy(out.data() + sizeof(resp), name.data(), name.size());
        }

        // Reply unicast back to requester
        ::sendto(sock_, out.data(), out.size(), 0, (sockaddr*)&src, sizeof(src));
    }
}

} // namespace lj