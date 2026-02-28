#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "virtual_gamepad.h"
#include "gamepad_mapping.h"

int fd;

bool VirtualGamepad::init() {
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "Cannot open uinput\n";
        return false;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);

    ioctl(fd, UI_SET_KEYBIT, GP_A);
    ioctl(fd, UI_SET_KEYBIT, GP_B);
    ioctl(fd, UI_SET_KEYBIT, GP_X);
    ioctl(fd, UI_SET_KEYBIT, GP_Y);

    ioctl(fd, UI_SET_KEYBIT, GP_L1);
    ioctl(fd, UI_SET_KEYBIT, GP_R1);

    ioctl(fd, UI_SET_KEYBIT, GP_START);
    ioctl(fd, UI_SET_KEYBIT, GP_SELECT);

    ioctl(fd, UI_SET_ABSBIT, GP_LX);
    ioctl(fd, UI_SET_ABSBIT, GP_LY);
    ioctl(fd, UI_SET_ABSBIT, GP_RX);
    ioctl(fd, UI_SET_ABSBIT, GP_RY);

    ioctl(fd, UI_SET_ABSBIT, GP_DPAD_X);
    ioctl(fd, UI_SET_ABSBIT, GP_DPAD_Y);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Virtual Gamepad");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    sleep(1);
    return true;
}

void VirtualGamepad::sendButton(int button, int value) {
    struct input_event ev{};
    ev.type = EV_KEY;
    ev.code = button;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

void VirtualGamepad::sendAxis(int axis, int value) {
    struct input_event ev{};
    ev.type = EV_ABS;
    ev.code = axis;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

void VirtualGamepad::sync() {
    struct input_event ev{};
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}