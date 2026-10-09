#pragma once
// Fake devices/usb.hpp: the host device table and delayed init hook xone_host.cpp touches
#include <array>
#include <memory>
#include "devices/usb/host/host.hpp"
extern std::array<std::shared_ptr<UsbHostDevice>, 127> host_devices;
void process_delayed_init();
