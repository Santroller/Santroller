#pragma once
#include "usb/Descriptors.h"
#include "controller_structs.h"
#include "../config/eeprom.h"

void output_init(void);
extern USB_HID_Descriptor_HID_t hid_descriptor;