#include "devices/bt/host/midi_host.hpp"
#include "protocols/ble_midi.hpp"

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

void BleMidiHost::handle_report(const uint8_t *data, uint16_t len)
{
    ble_midi_unwrap(data, len, [this](uint8_t byte)
                    { push(byte); });
    flush();
}
