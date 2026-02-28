#pragma once
#include <linux/input.h>

// --- FACE BUTTONS ---
#define GP_A BTN_SOUTH
#define GP_B BTN_EAST
#define GP_X BTN_WEST
#define GP_Y BTN_NORTH

// --- SHOULDER ---
#define GP_L1 BTN_TL
#define GP_R1 BTN_TR

// --- SYSTEM ---
#define GP_START BTN_START
#define GP_SELECT BTN_SELECT

// --- AXIS ---
#define GP_LX ABS_X
#define GP_LY ABS_Y
#define GP_RX ABS_RX
#define GP_RY ABS_RY

#define GP_L2 ABS_Z
#define GP_R2 ABS_RZ

// --- DPAD ---
#define GP_DPAD_X ABS_HAT0X
#define GP_DPAD_Y ABS_HAT0Y

// Thumbstick clicks
#define GP_L3 BTN_THUMBL
#define GP_R3 BTN_THUMBR

// System / menu
#define GP_HOME BTN_MODE