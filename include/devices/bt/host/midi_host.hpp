#pragma once
#include "devices/bt/bt_host.hpp"

// ---------------------------------------------------------------------------
// MIDI over Bluetooth LE
// Notifications on the MIDI I/O characteristic (UUID: 7772e5db-3868-4112-a1a9-f2669d106bf3),
// read through the MIDI channel inputs like a USB MIDI device
// ---------------------------------------------------------------------------
class BleMidiHost : public BluetoothHostInterface
{
public:
    BleMidiHost(uint16_t id);
    ~BleMidiHost() {}

    void handle_report(const uint8_t *data, uint16_t len) override;

    // Everything is read through the MIDI inputs instead
    bool tick_digital(proto_Output &type) override { return false; }
    uint16_t tick_analog(proto_Output &type) override { return 0; }

private:
    void push(uint8_t byte);
    void flush();
    uint8_t m_out[32];
    uint8_t m_out_len = 0;
    // room for a chord's worth of notes between polls
    MidiStaticBuffers<256, 0, 32, 1> m_ble_midi_buffers;
};
