#pragma once
// What the fakes recorded, for the tests to look at
#include <stdint.h>
#include <stddef.h>
#include <vector>

namespace fake_i2c_master
{
// One dmaWriteRead from the Wii extension reader
struct Transfer
{
    uint8_t addr;
    std::vector<uint8_t> written;
    uint8_t *read_into;
    size_t read_len;
};
extern std::vector<Transfer> transfers;
}

namespace fake_midi
{
// Every packet handed to MidiDevice::process_midi_data, as USB MIDI event packets
extern std::vector<std::vector<uint8_t>> packets;
}

void reset_wii_fakes();
