#pragma once
#include "virtual_gamepad.h"

class ControllerEngine {
public:
    ControllerEngine(VirtualGamepad* gp);
    void process(char* data, int len);

private:
    VirtualGamepad* gamepad;
};