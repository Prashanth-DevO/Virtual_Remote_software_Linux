#include "controller_engine.h"

#include <algorithm>
#include <cstdlib>
#include <unordered_map>

#include "gamepad_mapping.h"

namespace {
int clampInt(int value, int min_v, int max_v) {
    return std::max(min_v, std::min(value, max_v));
}
}  // namespace

ControllerEngine::ControllerEngine(VirtualGamepad* gp) : gamepad_(gp) {}

bool ControllerEngine::processButton(const std::string& key, const std::string& value) {
    static const std::unordered_map<std::string, int> button_map = {
        {"BTN_A", GP_A},       {"BTN_B", GP_B},     {"BTN_X", GP_X},
        {"BTN_Y", GP_Y},       {"BTN_L1", GP_L1},   {"BTN_R1", GP_R1},
        {"BTN_START", GP_START}, {"BTN_SELECT", GP_SELECT},
    };

    auto it = button_map.find(key);
    if (it == button_map.end()) {
        return false;
    }

    int pressed = std::atoi(value.c_str()) != 0 ? 1 : 0;
    gamepad_->sendButton(it->second, pressed);
    return true;
}

bool ControllerEngine::processAxis(const std::string& key, const std::string& value) {
    static const std::unordered_map<std::string, int> axis_map = {
        {"LX", GP_LX},       {"LY", GP_LY},       {"RX", GP_RX},
        {"RY", GP_RY},       {"DPAD_X", GP_DPAD_X}, {"DPAD_Y", GP_DPAD_Y},
        {"L2", GP_L2},       {"R2", GP_R2},
    };

    auto it = axis_map.find(key);
    if (it == axis_map.end()) {
        return false;
    }

    int raw = std::atoi(value.c_str());
    int final_value = raw;

    if (it->second == GP_DPAD_X || it->second == GP_DPAD_Y) {
        final_value = clampInt(raw, -1, 1);
    } else if (it->second == GP_L2 || it->second == GP_R2) {
        final_value = clampInt(raw, 0, 255);
    } else {
        final_value = clampInt(raw, -32768, 32767);
    }

    gamepad_->sendAxis(it->second, final_value);
    return true;
}

bool ControllerEngine::process(const char* data, int len) {
    if (data == nullptr || len <= 0) {
        return false;
    }

    std::string msg(data, len);
    auto sep = msg.find(':');
    if (sep == std::string::npos) {
        return false;
    }

    const std::string key = msg.substr(0, sep);
    const std::string value = msg.substr(sep + 1);

    if (processButton(key, value) || processAxis(key, value)) {
        gamepad_->sync();
        return true;
    }

    return false;
}