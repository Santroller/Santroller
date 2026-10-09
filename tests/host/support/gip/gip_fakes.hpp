#pragma once
// State behind the GIP area's link-time fakes (fakes.cpp): what went out over the fake USB
// device / host endpoints, and the knobs a test turns to play the console or controller.
#include <stdint.h>
#include <memory>
#include <vector>

class UsbHostInterface;

namespace gip_fake
{
struct Transfer
{
    uint8_t ep;
    std::vector<uint8_t> data;
};

// Emulated device side (TinyUSB device stack)
inline bool device_mounted = true;
inline bool device_suspended = false;
inline bool device_in_busy = false;
inline std::vector<Transfer> device_in;  // reports sent to the console on IN endpoints
inline int device_out_arms = 0;          // how many times an OUT transfer was queued

// USB host side
inline std::vector<Transfer> host_out;   // reports sent to the controller (send_intr_xfer)
inline bool host_out_fail = false;       // make send_intr_xfer fail (endpoint busy)
inline uint8_t *host_in_buffer = nullptr; // buffer the IN endpoint was last armed with
inline uint16_t host_in_size = 0;
inline int host_in_arms = 0;
inline std::vector<UsbHostInterface *> enumerating;
inline std::vector<std::shared_ptr<UsbHostInterface>> assignable;
inline int delayed_init_calls = 0;

void reset();
} // namespace gip_fake
