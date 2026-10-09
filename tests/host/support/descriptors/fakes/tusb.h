#pragma once
// Fake tusb.h for the descriptor tests: TinyUSB's HID definitions and descriptor templates, plus the
// device stack declarations spice2x_device.cpp uses, without the Pico OS layer.
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "device/usbd.h"
// tud_suspended() / tud_mounted() are defined in spice2x_descriptor.cpp
