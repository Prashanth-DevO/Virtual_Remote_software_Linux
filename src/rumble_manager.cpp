#include "rumble_manager.h"

void RumbleManager::storeRumbleEffect(int effect_id, uint16_t strong, uint16_t weak, uint16_t duration_ms) {
    std::lock_guard<std::mutex> lock(effects_mutex_);
    effects_[effect_id] = {strong, weak, duration_ms, true};
}

bool RumbleManager::getRumbleEffect(int effect_id, RumbleEffect& out) {
    std::lock_guard<std::mutex> lock(effects_mutex_);
    auto it = effects_.find(effect_id);
    if (it == effects_.end() || !it->second.valid) {
        return false;
    }
    out = it->second;
    return true;
}

void RumbleManager::eraseRumbleEffect(int effect_id) {
    std::lock_guard<std::mutex> lock(effects_mutex_);
    effects_.erase(effect_id);
}