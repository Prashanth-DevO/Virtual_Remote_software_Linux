#include "virtual_gamepad.h"

#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>


#include "gamepad_mapping.h"

VirtualGamepad::VirtualGamepad(TcpFeedbackServer* feedback_server)
    : fd_(-1), feedback_server_(feedback_server) {}

VirtualGamepad::~VirtualGamepad() {
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
    }
}

bool VirtualGamepad::setupAbsAxis(int axis, int min, int max) {
    if (ioctl(fd_, UI_SET_ABSBIT, axis) < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    struct uinput_abs_setup abs_setup {};
    abs_setup.code = axis;
    abs_setup.absinfo.minimum = min;
    abs_setup.absinfo.maximum = max;
    int range = max - min;
    abs_setup.absinfo.flat = (range <= 2) ? 0 : 16;
    abs_setup.absinfo.fuzz = 0;
    abs_setup.absinfo.value = 0;

    if (ioctl(fd_, UI_ABS_SETUP, &abs_setup) < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    return true;
}

bool VirtualGamepad::init() {
    fd_ = open("/dev/uinput", O_RDWR | O_NONBLOCK);
    if (fd_ < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(fd_, UI_SET_EVBIT, EV_ABS) < 0 || ioctl(fd_, UI_SET_EVBIT, EV_FF) <0 ||ioctl(fd_, UI_SET_FFBIT, FF_RUMBLE)< 0 ) {
        last_error_ = std::strerror(errno);
        return false;
    }

    int buttons[] = {GP_A, GP_B, GP_X, GP_Y, GP_L1, GP_R1, GP_START, GP_SELECT, GP_L3, GP_R3, GP_HOME};
    for (int button : buttons) {
        if (ioctl(fd_, UI_SET_KEYBIT, button) < 0) {
            last_error_ = std::strerror(errno);
            std::cout << "Failed to set button bit for button " << button << ": " << last_error_ << "\n";
            return false;
        }
    }

    if (!setupAbsAxis(GP_LX, -32768, 32767) || !setupAbsAxis(GP_LY, -32768, 32767) ||
        !setupAbsAxis(GP_RX, -32768, 32767) || !setupAbsAxis(GP_RY, -32768, 32767) ||
        !setupAbsAxis(GP_DPAD_X, -1, 1) || !setupAbsAxis(GP_DPAD_Y, -1, 1) ||
        !setupAbsAxis(GP_L2, 0, 255) || !setupAbsAxis(GP_R2, 0, 255)) {
            std::cout << "Failed to setup axes: " << last_error_ << "\n";
        return false;
    }

    struct uinput_setup usetup {};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    usetup.ff_effects_max = 16; 
    std::strncpy(usetup.name, "Virtual Remote Gamepad", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd_, UI_DEV_SETUP, &usetup) < 0) {
        last_error_ = std::string("UI_DEV_SETUP failed: ") + std::strerror(errno);
        return false;
    }

    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        last_error_ = std::string("UI_DEV_CREATE failed: ") + std::strerror(errno);
        return false;
    }
    usleep(250000);
    return true;
}

void VirtualGamepad::sendButton(int button, int value) {
    struct input_event ev {};
    ev.type = EV_KEY;
    ev.code = button;
    ev.value = value;
    write(fd_, &ev, sizeof(ev));
}

void VirtualGamepad::sendAxis(int axis, int value) {
    struct input_event ev {};
    ev.type = EV_ABS;
    ev.code = axis;
    ev.value = value;
    write(fd_, &ev, sizeof(ev));
}

void VirtualGamepad::sync() {
    struct input_event ev {};
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd_, &ev, sizeof(ev));
}

std::string VirtualGamepad::lastError() const {
    return last_error_;
}

int VirtualGamepad::mapRumbleToStrength(uint16_t strong, uint16_t weak) {
    uint16_t combined = std::max(strong, weak);
    return (combined * 255) / 65535;
}

void VirtualGamepad::handlePlayEffect(int effect_id) {
    RumbleEffect effect;
    if (!rumble_manager_.getRumbleEffect(effect_id, effect)) {
        return;
    }

    int strength = mapRumbleToStrength(effect.strong, effect.weak);

    std::cout << "[FF] play effect id=" << effect_id
              << " strong=" << effect.strong
              << " weak=" << effect.weak
              << " duration=" << effect.duration_ms
              << " mapped_strength=" << strength << "\n";

    if (feedback_server_) {
        feedback_server_->sendVibration(effect.duration_ms, strength);
    }
}

void VirtualGamepad::processForceFeedback() {
    while (true) {
        struct input_event ev {};
        ssize_t n = read(fd_, &ev, sizeof(ev));

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cout << "[FF] read failed: " << std::strerror(errno) << "\n";
            break;
        }

        if (n != sizeof(ev)) {
            continue;
        }

        if (ev.type == EV_FF) {
            if (ev.value > 0) {
                handlePlayEffect(ev.code);
            } else if (ev.value == 0) {
                if (feedback_server_) {
                    feedback_server_->sendVibration(0, 0);
                }
            }
        }
    }
}
