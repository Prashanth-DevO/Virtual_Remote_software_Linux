#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>

struct RumbleEffect {
    uint16_t strong;
    uint16_t weak;
    uint16_t duration_ms;
    bool valid;
};

class RumbleManager {
public:
    void storeRumbleEffect(int effect_id, uint16_t strong, uint16_t weak, uint16_t duration_ms);
    bool getRumbleEffect(int effect_id, RumbleEffect& out);
    void eraseRumbleEffect(int effect_id);

private:
    std::unordered_map<int, RumbleEffect> effects_;
    std::mutex effects_mutex_;
};