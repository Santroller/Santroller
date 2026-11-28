#pragma once
#include "base.hpp"
#include "device.pb.h"
#include <vector>

// One pad of a drum kit: the input it reads, by the id DrumKitInputs pick it by
struct DrumKitPad
{
    uint32_t id;
    proto_Input input;
};

// Drum pads set up once for every drum layout. The kit only holds where each pad is: a DrumKitInput reads its
// pad's input as if the mapping had it itself, so the pad gets set up and read like any other input.
class DrumKitDevice : public Device
{
public:
    // More pads than this are dropped, so a broken config can't run out of RAM
    static constexpr size_t MAX_PADS = 32;
    DrumKitDevice(uint16_t id) : Device(id) {}
    void begin() {}
    void end(bool full) {}
    void update(bool full_poll, bool send_events) {}
    // The pads come from a callback while the config is decoded (see load_drum_pad), not the device's config
    void set_pads(std::vector<DrumKitPad> pads) { m_pads = std::move(pads); }
    bool using_pin(uint8_t pin)
    {
        for (const auto &pad : m_pads)
        {
            if (pad.input.which_input == proto_Input_gpio_tag && pad.input.input.gpio.pin == pin)
            {
                return true;
            }
        }
        return false;
    }
    // The pad's input, or nullptr when there's no pad with this id
    const proto_Input *pad(uint32_t id) const
    {
        for (const auto &pad : m_pads)
        {
            if (pad.id == id)
            {
                return &pad.input;
            }
        }
        return nullptr;
    }

private:
    std::vector<DrumKitPad> m_pads;
};
