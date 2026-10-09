#pragma once
#include <stdint.h>

// MIDI over Bluetooth LE. A packet is a header byte, then messages that each start with a timestamp
// byte. Both have the top bit set, so a timestamp is followed by either a status byte or, for running
// status, data bytes. Sysex can carry on into the next packet, which then has data bytes straight after
// the header.

// BLE MIDI I/O characteristic UUID: 7772e5db-3868-4112-a1a9-f2669d106bf3
static const uint8_t ble_midi_char_uuid[16] = {
    0x77, 0x72, 0xE5, 0xDB, 0x38, 0x68, 0x41, 0x12,
    0xA1, 0xA9, 0xF2, 0x66, 0x9D, 0x10, 0x6B, 0xF3};
// BLE MIDI service UUID 03b80e5a-ede8-4b33-a751-6ce34ec4c700, little-endian as it is advertised
static const uint8_t ble_midi_service_uuid_le[16] = {
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7,
    0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03};

// Strip the header and timestamps, passing the plain MIDI bytes to emit in order.
// Packets without a valid header are dropped.
template <typename Emit>
static inline void ble_midi_unwrap(const uint8_t *data, uint16_t len, Emit &&emit)
{
    if (len < 2 || (data[0] & 0xC0) != 0x80)
    {
        return;
    }
    uint16_t i = 1;
    while (i < len)
    {
        if (data[i] & 0x80)
        {
            // timestamp, we only care about the order things happen in
            i++;
            if (i < len && (data[i] & 0x80))
            {
                emit(data[i++]);
            }
        }
        while (i < len && !(data[i] & 0x80))
        {
            emit(data[i++]);
        }
    }
}
