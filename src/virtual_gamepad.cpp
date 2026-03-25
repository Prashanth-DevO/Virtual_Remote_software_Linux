#include "../include/virtual_gamepad.h"

#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "../include/gamepad_mapping.h"

VirtualGamepad::VirtualGamepad() : fd_(-1) {}

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
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(fd_, UI_SET_EVBIT, EV_ABS) < 0) {
        last_error_ = std::strerror(errno);
        return false;
    }

    int buttons[] = {GP_A, GP_B, GP_X, GP_Y, GP_L1, GP_R1, GP_START, GP_SELECT, GP_L3, GP_R3, GP_HOME};
    for (int button : buttons) {
        if (ioctl(fd_, UI_SET_KEYBIT, button) < 0) {
            last_error_ = std::strerror(errno);
            return false;
        }
    }

    if (!setupAbsAxis(GP_LX, -32768, 32767) || !setupAbsAxis(GP_LY, -32768, 32767) ||
        !setupAbsAxis(GP_RX, -32768, 32767) || !setupAbsAxis(GP_RY, -32768, 32767) ||
        !setupAbsAxis(GP_DPAD_X, -1, 1) || !setupAbsAxis(GP_DPAD_Y, -1, 1) ||
        !setupAbsAxis(GP_L2, 0, 255) || !setupAbsAxis(GP_R2, 0, 255)) {
        return false;
    }

    struct uinput_setup usetup {};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    std::strncpy(usetup.name, "Virtual Remote Gamepad", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd_, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd_, UI_DEV_CREATE) < 0) {
        last_error_ = std::strerror(errno);
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