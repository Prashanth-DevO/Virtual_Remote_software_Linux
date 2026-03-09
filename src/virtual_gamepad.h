#pragma once

#include <string>

class VirtualGamepad {
public:
    VirtualGamepad();
    ~VirtualGamepad();

    bool init();
    void sendButton(int button, int value);
    void sendAxis(int axis, int value);
    void sync();

    std::string lastError() const;

private:
    bool setupAbsAxis(int axis, int min, int max);

    int fd_;
    std::string last_error_;
};