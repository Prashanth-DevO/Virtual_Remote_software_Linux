#pragma once

#include <string>

#include "virtual_gamepad.h"

class ControllerEngine {
public:
    explicit ControllerEngine(VirtualGamepad* gp);

    // Returns true if a control packet was handled.
    bool process(const char* data, int len);

private:
    bool processButton(const std::string& key, const std::string& value);
    bool processAxis(const std::string& key, const std::string& value);

    VirtualGamepad* gamepad_;
};