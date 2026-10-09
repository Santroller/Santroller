// Link time fakes for what the mapping sources call outside src/mappings
#include "profiles/profile.hpp"
#include "pico/time.h"
#include "mappings/base_mapping.hpp"
#include "devices/midi.hpp"
#include "leds/led_mappings.hpp"
#include "triggers/activation_trigger_list.hpp"
#include "triggers/activation_trigger.hpp"

Profile::~Profile() {}

// Same as src/profiles/profile.cpp
bool Profile::other_strum_live(int8_t strum_bit) const
{
    for (auto *mapping : strum_mappings)
    {
        if (mapping->strum_bit() != strum_bit && mapping->live_value())
        {
            return true;
        }
    }
    return false;
}

// Same as src/profiles/profile.cpp
void Profile::reset_drum_state()
{
    drum_state.end_report(time_us_64());
    drum_state.reset();
}

// No MIDI devices exist in these tests
uint8_t MidiDevice::read_midi_note(uint8_t channel, uint8_t note) const
{
    (void)channel;
    (void)note;
    return 0;
}

// Profiles in these tests have no activation triggers
ActivationTriggerList::~ActivationTriggerList() = default;
