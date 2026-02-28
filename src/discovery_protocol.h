#pragma once
#include <cstdint>

namespace lj {

// "LJDS" as little-endian uint32: 'L' 'J' 'D' 'S'
static constexpr uint32_t kDiscoveryMagic = 0x53444A4C;
static constexpr uint8_t  kDiscoveryVersion = 1;

enum class DiscMsgType : uint8_t {
    DiscoverReq  = 1,
    DiscoverResp = 2,
};

#pragma pack(push, 1)
struct DiscoverReqV1 {
    uint32_t magic;     // kDiscoveryMagic
    uint8_t  version;   // kDiscoveryVersion
    uint8_t  msg_type;  // DiscMsgType::DiscoverReq
    uint16_t reserved;  // 0
    uint32_t nonce;     // random from client
};

struct DiscoverRespV1 {
    uint32_t magic;       // kDiscoveryMagic
    uint8_t  version;     // kDiscoveryVersion
    uint8_t  msg_type;    // DiscMsgType::DiscoverResp
    uint16_t reserved;    // 0
    uint32_t nonce;       // copied from request
    uint64_t server_id;   // stable-ish
    uint16_t control_port;// 9000
    uint16_t feedback_port;// 9001
    uint16_t proto_ver;   // your ControllerPacketV1 version (e.g. 1)
    uint16_t name_len;    // bytes following (max 64)
    uint32_t flags;       // bitmask
    // followed by name bytes (not null-terminated)
};
#pragma pack(pop)

static constexpr uint16_t kMaxNameLen = 64;

// flags
enum DiscFlags : uint32_t {
    kFlagSupportsFeedback = 1u << 0,
    kFlagSupportsRumble   = 1u << 1,
    kFlagPairedLocked     = 1u << 2,
};

} // namespace lj