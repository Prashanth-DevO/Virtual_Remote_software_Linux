#pragma once

class VirtualGamepad {
public:
    bool init();
    void sendButton(int button, int value);
    void sendAxis(int axis, int value);
    void sync();
};