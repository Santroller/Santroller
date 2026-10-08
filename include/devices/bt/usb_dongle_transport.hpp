#pragma once
#include "hci_transport.h"

// BTstack HCI transport for a USB bluetooth adapter on the host port, used when there is no CYW43
const hci_transport_t *usb_dongle_transport_instance();
// BTstack hardware error callback for the adapter
void usb_dongle_hardware_error(uint8_t error);
// True once, when HCI should be restarted (the adapter was unplugged, or reported a hardware error)
bool usb_dongle_take_restart();
