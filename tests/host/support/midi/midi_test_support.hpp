#pragma once
#include <stdint.h>
#include <deque>
#include <vector>
#include <initializer_list>
#include "protocols/midi_input.hpp"

namespace midi_test
{
using Bytes = std::vector<uint8_t>;

// Stands in for the endpoint's rx FIFO. Like tu_fifo_peek_n / tu_fifo_read_n, peek_n and read only copy
// what is there and return how much that was.
class FakeMidiStream
{
public:
    void write(const Bytes &bytes) { m_data.insert(m_data.end(), bytes.begin(), bytes.end()); }
    size_t available() const { return m_data.size(); }

    bool peek(uint8_t &byte)
    {
        if (m_data.empty())
        {
            return false;
        }
        byte = m_data.front();
        return true;
    }
    uint32_t peek_n(void *buffer, uint16_t len)
    {
        uint32_t count = len < m_data.size() ? len : m_data.size();
        for (uint32_t i = 0; i < count; i++)
        {
            static_cast<uint8_t *>(buffer)[i] = m_data[i];
        }
        return count;
    }
    uint32_t read(void *buffer, uint32_t len)
    {
        uint32_t count = peek_n(buffer, len);
        m_data.erase(m_data.begin(), m_data.begin() + count);
        return count;
    }

private:
    std::deque<uint8_t> m_data;
};

// USB MIDI 1.0 event packets (USB Device Class Definition for MIDI Devices, 4: USB-MIDI Event Packets).
// Byte 0 is the cable number in the high nibble and the Code Index Number in the low nibble, then up to
// three MIDI bytes padded with zeros.
inline Bytes usb_packet(uint8_t cable, uint8_t cin, Bytes midi)
{
    midi.resize(3, 0);
    return {static_cast<uint8_t>((cable << 4) | cin), midi[0], midi[1], midi[2]};
}

// Code Index Number for a complete message other than sysex (table 4-1)
inline uint8_t usb_cin(const Bytes &message)
{
    uint8_t status = message[0];
    if (status < 0xF0)
    {
        // channel voice messages use the high nibble of the status
        return status >> 4;
    }
    switch (status)
    {
    case 0xF1: // MTC quarter frame
    case 0xF3: // song select
        return 0x2;
    case 0xF2: // song position pointer
        return 0x3;
    case 0xF6: // tune request is a single byte system common message
        return 0x5;
    default:
        // realtime messages are single bytes
        return 0xF;
    }
}

// A sysex message as CIN 4 (start / continue) packets, ending with CIN 5, 6 or 7 for the last 1, 2 or 3 bytes
inline Bytes usb_sysex(uint8_t cable, const Bytes &sysex)
{
    Bytes out;
    size_t i = 0;
    while (sysex.size() - i > 3)
    {
        Bytes packet = usb_packet(cable, 0x4, Bytes(sysex.begin() + i, sysex.begin() + i + 3));
        out.insert(out.end(), packet.begin(), packet.end());
        i += 3;
    }
    size_t remaining = sysex.size() - i;
    Bytes packet = usb_packet(cable, 0x4 + remaining, Bytes(sysex.begin() + i, sysex.end()));
    out.insert(out.end(), packet.begin(), packet.end());
    return out;
}

// Any complete message as USB MIDI packets
inline Bytes usb_message(uint8_t cable, const Bytes &message)
{
    if (message[0] == 0xF0)
    {
        return usb_sysex(cable, message);
    }
    return usb_packet(cable, usb_cin(message), message);
}

// A MidiInput fed from a FakeMidiStream, recording what it reports back
class MidiHarness
{
public:
    explicit MidiHarness(bool usb_packets, uint8_t max_cables = 1) : midi(usb_packets)
    {
        midi.set_max_cables(max_cables);
    }

    // Add bytes to the FIFO and parse them, like one transfer followed by an update()
    void feed(const Bytes &bytes)
    {
        stream.write(bytes);
        parse();
    }
    void parse()
    {
        midi.parse(
            stream,
            [this](const uint8_t *data, uint8_t size)
            { messages.emplace_back(data, data + size); },
            [this](uint8_t channel)
            {
                // MidiDevice only reloads for channels it hasn't seen before
                marked.push_back(channel);
                if (midi.set_channel_seen(channel))
                {
                    newly_seen.push_back(channel);
                }
            });
    }

    MidiInput midi;
    FakeMidiStream stream;
    // complete messages, as passed to the MIDI monitor
    std::vector<Bytes> messages;
    // every mark_channel_seen call, and the ones that were for a new channel
    std::vector<uint8_t> marked;
    std::vector<uint8_t> newly_seen;
};
} // namespace midi_test
