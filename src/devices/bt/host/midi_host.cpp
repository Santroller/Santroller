#include "devices/bt/host/midi_host.hpp"

// ============================================================================
// BleMidiHost (MIDI over Bluetooth LE)
// ============================================================================

BleMidiHost::BleMidiHost(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Midi;
    init_buffers(m_ble_midi_buffers.config());
    // BLE MIDI unwraps to a plain MIDI byte stream, not USB MIDI packets
    set_usb_packets(false);
}

void BleMidiHost::push(uint8_t byte)
{
    m_out[m_out_len++] = byte;
    if (m_out_len == sizeof(m_out))
    {
        flush();
    }
}

void BleMidiHost::flush()
{
    if (m_out_len)
    {
        process_midi_data(m_out, m_out_len);
        m_out_len = 0;
    }
}

// A BLE MIDI packet is a header byte, then messages that each start with a timestamp byte. Both have
// the top bit set, so a timestamp is followed by either a status byte or, for running status, data
// bytes. Sysex can carry on into the next packet, which then has data bytes straight after the header.
void BleMidiHost::handle_report(const uint8_t *data, uint16_t len)
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
                push(data[i++]);
            }
        }
        while (i < len && !(data[i] & 0x80))
        {
            push(data[i++]);
        }
    }
    flush();
}
