#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "midi_serial.hpp"
#include "midi.hpp"
#include "protocols/midi_output.hpp"
class MidiSerialDevice : public MidiDevice
{
public:
    ~MidiSerialDevice() {}
    MidiSerialDevice(const DeviceReloadState* state, proto_SerialMidiDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool matches_reload_config(const proto_Device &config) const override;
    bool using_pin(uint8_t pin);
    // A MIDI out jack is only ever written to, so mappings can't read it
    bool is_midi_device() const override { return !is_output(); }
    MidiSerial serial;

private:
    static bool is_output(const proto_SerialMidiDevice &device)
    {
        return device.has_direction && device.direction == MidiSerialOut;
    }
    bool is_output() const { return is_output(m_device); }
    void send_output();
    proto_SerialMidiDevice m_device;
    MidiStaticBuffers<32, 0, 32, 1> m_midi_buffers;
    MidiState m_state;
    MidiOutput m_output;
    // The message being written out, a byte at a time as the TX FIFO has room
    uint8_t m_tx[3] = {};
    uint8_t m_tx_len = 0;
    uint8_t m_tx_pos = 0;
    // 0 when the next message has to send its status byte
    uint8_t m_running_status = 0;
    uint64_t m_last_tx_us = 0;
};
