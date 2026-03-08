#pragma once

#include <string>
#include <rumble_manager.h>
#include <tcp_feedback_server.h>

class VirtualGamepad {
public:
    ~VirtualGamepad();
    explicit VirtualGamepad(TcpFeedbackServer* feedback_server = nullptr);
    bool init();
    void sendButton(int button, int value);
    void sendAxis(int axis, int value);
    void sync();

    std::string lastError() const;
    static int mapRumbleToStrength(uint16_t strong, uint16_t weak);
    void handlePlayEffect(int effect_id);
    void processForceFeedback();
private:
    bool setupAbsAxis(int axis, int min, int max);
    TcpFeedbackServer* feedback_server_;
    RumbleManager rumble_manager_;
    int fd_;
    std::string last_error_;
};
