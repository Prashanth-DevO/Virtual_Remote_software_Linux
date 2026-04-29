#include "../include/discovery_service.h"
#include "../include/discovery_protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <fstream>
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
        id.erase(std::remove_if(id.begin(), id.end(),
                                [](unsigned char c) { return std::isspace(c) != 0; }),
                 id.end());

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

static uint64_t bswap64_u(uint64_t v) {
    return ((v & 0x00000000000000FFull) << 56) |
           ((v & 0x000000000000FF00ull) << 40) |
           ((v & 0x0000000000FF0000ull) << 24) |
           ((v & 0x00000000FF000000ull) << 8) |
           ((v & 0x000000FF00000000ull) >> 8) |
           ((v & 0x0000FF0000000000ull) >> 24) |
           ((v & 0x00FF000000000000ull) >> 40) |
           ((v & 0xFF00000000000000ull) >> 56);
}

static uint64_t htonll_u(uint64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bswap64_u(v);
#else
    return v;
#endif
}

static std::string makeLegacyReply(const ServerStatus& st) {
    return "CONTROLLER_AVAILABLE;udp=" + std::to_string(st.control_port);
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
        // std::cerr << "[discovery] socket() failed: " << strerror(errno) << "\n";
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
        // std::cerr << "[discovery] bind() failed on port " << port_
        //           << ": " << strerror(errno) << "\n";
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
    constexpr char kLegacyQuery[] = "WHO_IS_CONTROLLER";
    constexpr size_t kLegacyQueryLen = sizeof(kLegacyQuery) - 1;

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

        // Rate limit by source IPv4
        uint32_t ipKey = src.sin_addr.s_addr; // network order is fine as a key
        auto now = Clock::now();
        auto it = lastReply.find(ipKey);
        if (it != lastReply.end() && (now - it->second) < minInterval) {
            continue;
        }

        // Build response from status provider
        ServerStatus st = status_fn_ ? status_fn_() : ServerStatus{};
        lastReply[ipKey] = now;

        // Backward compatibility: old text-based discovery request.
        if ((size_t)n == kLegacyQueryLen &&
            std::memcmp(buf.data(), kLegacyQuery, kLegacyQueryLen) == 0) {
            const std::string reply = makeLegacyReply(st);
            ::sendto(sock_, reply.data(), reply.size(), 0, (sockaddr*)&src, sizeof(src));
            continue;
        }

        // Strict minimum size check for binary discovery request.
        if ((size_t)n < sizeof(DiscoverReqV1)) continue;

        DiscoverReqV1 req{};
        std::memcpy(&req, buf.data(), sizeof(req));

        // Accept both host-order (legacy implementation behavior) and network-order
        // client packets to avoid silent interoperability failures.
        bool request_network_order = false;
        uint32_t req_nonce = req.nonce;
        if (req.magic == kDiscoveryMagic) {
            request_network_order = false;
        } else if (ntohl(req.magic) == kDiscoveryMagic) {
            request_network_order = true;
            req_nonce = ntohl(req.nonce);
        } else {
            continue;
        }
        if (req.version != kDiscoveryVersion) continue;
        if (req.msg_type != (uint8_t)DiscMsgType::DiscoverReq) continue;

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
        resp.nonce = req_nonce;
        resp.server_id = st.server_id;
        resp.control_port = st.control_port;
        resp.reserved_port = 0;
        resp.proto_ver = st.controller_proto_ver;
        resp.name_len = (uint16_t)name.size();
        resp.flags = flags;

        if (request_network_order) {
            resp.magic = htonl(resp.magic);
            resp.reserved = htons(resp.reserved);
            resp.nonce = htonl(resp.nonce);
            resp.server_id = htonll_u(resp.server_id);
            resp.control_port = htons(resp.control_port);
            resp.reserved_port = htons(resp.reserved_port);
            resp.proto_ver = htons(resp.proto_ver);
            resp.name_len = htons(resp.name_len);
            resp.flags = htonl(resp.flags);
        }

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
