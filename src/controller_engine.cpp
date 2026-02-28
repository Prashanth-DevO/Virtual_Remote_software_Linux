#include "controller_engine.h"
#include "gamepad_mapping.h"
#include <string>

ControllerEngine::ControllerEngine(VirtualGamepad* gp) {
    gamepad = gp;
}

void ControllerEngine::process(char* data, int len) {

    std::string msg(data, len);

    if(msg == "BTN_A:1") {
        gamepad->sendButton(GP_A, 1);
        gamepad->sync();
    }

    else if(msg == "BTN_A:0") {
        gamepad->sendButton(GP_A, 0);
        gamepad->sync();
    }

    else if(msg == "BTN_B:1") {
        gamepad->sendButton(GP_B, 1);
        gamepad->sync();
    }

    else if(msg == "BTN_B:0") {
        gamepad->sendButton(GP_B, 0);
        gamepad->sync();
    }

    else if(msg.find("LX:") == 0) {
        int value = std::stoi(msg.substr(3));
        gamepad->sendAxis(GP_LX, value);
        gamepad->sync();
    }

    else if(msg.find("LY:") == 0) {
        int value = std::stoi(msg.substr(3));
        gamepad->sendAxis(GP_LY, value);
        gamepad->sync();
    }
}