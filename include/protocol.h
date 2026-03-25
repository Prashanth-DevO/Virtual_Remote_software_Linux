#pragma once
#include <cstdint>

// Force tight layout (no padding) so UDP bytes match exactly.
#pragma pack(push, 1)

struct ControllerPacketV1 {
    uint32_t magic;        // 'UNIO' = 0x4F494E55 (little-endian in memory on x86)
    uint16_t version;      // 1
    uint16_t size;         // sizeof(ControllerPacketV1)

    uint32_t seq;          // incrementing sequence number
    uint64_t ts_us;        // timestamp (microseconds)

    int16_t  lx, ly;       // -32768..32767
    int16_t  rx, ry;       // -32768..32767

    uint8_t  l2, r2;       // 0..255
    int8_t   dpad_x;       // -1,0,1
    int8_t   dpad_y;       // -1,0,1

    uint32_t buttons;      // bitmask (see mapping below)
};

#pragma pack(pop)

// Magic constant for sanity check
static constexpr uint32_t UNIO_MAGIC = 0x4F494E55; // 'U''N''I''O'
static constexpr uint16_t UNIO_V1    = 1;

// Button bit positions (buttons bitmask)
enum ButtonBits : uint32_t {
    BTNBIT_A      = 0,
    BTNBIT_B      = 1,
    BTNBIT_X      = 2,
    BTNBIT_Y      = 3,
    BTNBIT_L1     = 4,
    BTNBIT_R1     = 5,
    BTNBIT_L3     = 6,
    BTNBIT_R3     = 7,
    BTNBIT_SELECT = 8,
    BTNBIT_START  = 9,
    BTNBIT_HOME   = 10,
};

static_assert(sizeof(ControllerPacketV1) == 36, "ControllerPacketV1 size must be 36 bytes");