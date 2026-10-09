// Spice2x's report descriptor, from the real spice2x_device.cpp (built against fakes/): initialize()
// copies its static descriptor and patches the LED string indices in, and that's what the device serves.
#include <vector>
#include "emulation/usb/spice2x_device.h"
#include "firmware_descriptors.hpp"

// The device stack calls Spice2xDevice::process() makes (never called here)
extern "C" bool tud_suspended(void) { return false; }
extern "C" bool tud_mounted(void) { return false; }

namespace hid_desc
{
std::vector<uint8_t> spice2x_descriptor()
{
    Spice2xDevice device;
    device.initialize();
    const uint8_t *desc = device.report_descriptor();
    return std::vector<uint8_t>(desc, desc + device.report_desc_len());
}
} // namespace hid_desc
