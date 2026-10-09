#include "devices/midi.hpp"
#include "devices/usb/host/host.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"

#include "events.pb.h"
#include "emulation/usb/hid_device.h"
#include "devices/usb/host/hid/hid_host.h"
#include "main.hpp"
#include "config/config.hpp"
void MidiDevice::init_buffers(const MidiBufferConfig &buffer_config)
{
    tu_edpt_stream_deinit(&ep_stream.rx);
    tu_edpt_stream_deinit(&ep_stream.tx);
    tu_memclr(&ep_stream, sizeof(ep_stream));
    if (buffer_config.rx_ff_buf && buffer_config.rx_ff_bufsize && buffer_config.m_ep_in_buf)
    {
        tu_edpt_stream_init(&ep_stream.rx, true, false, false,
                            buffer_config.rx_ff_buf, buffer_config.rx_ff_bufsize, buffer_config.m_ep_in_buf);
    }
    if (buffer_config.tx_ff_buf && buffer_config.tx_ff_bufsize && buffer_config.m_ep_out_buf)
    {
        tu_edpt_stream_init(&ep_stream.tx, true, true, false,
                            buffer_config.tx_ff_buf, buffer_config.tx_ff_bufsize, buffer_config.m_ep_out_buf);
    }
    m_midi.set_max_cables(buffer_config.max_cables);
}

MidiDevice::MidiDevice(const DeviceReloadState* state, uint16_t id, bool usbBased, const MidiBufferConfig &buffer_config) : Device(id), m_midi(usbBased), usbBased(usbBased)
{
    init_buffers(buffer_config);
    if (state) {
        m_midi.load_seen_channels(state->seen_midi_channels);
    }
}

MidiDevice::~MidiDevice()
{
    printf("MIDI Device destroyed\r\n");
    tu_edpt_stream_deinit(&ep_stream.rx);
    tu_edpt_stream_deinit(&ep_stream.tx);
}

void MidiDevice::save_reload_state(DeviceReloadState& state) const
{
    state.valid = true;
    m_midi.save_seen_channels(state.seen_midi_channels);
}

bool MidiDevice::mark_channel_seen(uint8_t channel)
{
    if (!m_midi.set_channel_seen(channel))
    {
        return false;
    }

    printf("Seen new MIDI channel: %d on device %d\r\n", channel, m_id);
    reload();
    return true;
}

void MidiDevice::rescan(bool first)
{
    if (!first || usbBased)
    {
        return;
    }

    for (int i = 0; i < 18; i++)
    {
        if (m_midi.has_midi_channel(i))
        {
            // Non-USB MIDI devices restore channel slots from the root device.
            DeviceManager::instance().add_assignable_device(std::static_pointer_cast<MidiDevice>(DeviceManager::instance().get_root_device(m_id)));
            printf("Assigning MIDI channel: %d on device %d\r\n", i, m_id);
        }
    }
}

void MidiDevice::process_midi_data(uint8_t *data, uint16_t len)
{
    tu_edpt_stream_t *ep_str_rx = &ep_stream.rx;
    memcpy(ep_str_rx->ep_buf, data, len);
    tu_edpt_stream_read_xfer_complete(ep_str_rx, len);
}
// Reads the endpoint's rx FIFO for MidiInput
struct MidiEndpointStream
{
    tu_edpt_stream_t *s;
    bool peek(uint8_t &byte) { return tu_edpt_stream_peek(s, &byte); }
    uint32_t peek_n(void *buffer, uint16_t len) { return tu_fifo_peek_n(&s->ff, buffer, len); }
    uint32_t read(void *buffer, uint32_t len) { return tu_edpt_stream_read(s, buffer, len); }
};

void MidiDevice::update(bool full_poll, bool send_events)
{
    MidiEndpointStream stream = {&ep_stream.rx};
    m_midi.parse(
        stream,
        [](const uint8_t *data, uint8_t size)
        {
            // timing clock and active sensing are sent continuously and would otherwise flood the monitor
            if (data[0] != MIDI_STATUS_SYSREAL_TIMING_CLOCK &&
                data[0] != MIDI_STATUS_SYSREAL_ACTIVE_SENSING)
            {
                proto_Event event = {which_event : proto_Event_midiDebug_tag, event : {midiDebug : {data : {size : size, bytes : {0}}}}};
                memcpy(event.event.midiDebug.data.bytes, data, size);
                HIDConfigDevice::send_event(event, false);
            }
        },
        [this](uint8_t channel)
        { mark_channel_seen(channel); });
}

bool MidiDevice::consume_midi_note_event(uint8_t channel, uint8_t note, uint16_t &sequence, uint16_t &velocity)
{
    return m_midi.consume_midi_note_event(channel, note, sequence, velocity);
}

bool MidiDevice::peek_midi_note_event(uint8_t channel, uint8_t note, uint16_t &sequence, uint16_t &velocity) const
{
    return m_midi.peek_midi_note_event(channel, note, sequence, velocity);
}

uint16_t MidiDevice::read_midi_control_change(uint8_t channel, uint8_t cc)
{
    return m_midi.read_midi_control_change(channel, cc);
}
uint8_t MidiDevice::read_midi_note(uint8_t channel, uint8_t note) const
{
    return m_midi.read_midi_note(channel, note);
}
bool MidiDevice::is_midi_note_pressed(uint8_t channel, uint8_t note) const
{
    return read_midi_note(channel, note) > 0;
}
uint16_t MidiDevice::read_midi_pitch_bend(uint8_t channel)
{
    return m_midi.read_midi_pitch_bend(channel);
}

bool MidiDevice::read_pro_guitar_button(proto_ProGuitarMidiButtonType button)
{
    const ProGuitar_Sysex_Buttons_t &midiButtons = m_midi.pro_guitar_buttons();
    const auto &midiFrets = m_midi.pro_guitar_frets();
    uint8_t dpad = midiButtons.dpad >= 0x08 ? 0 : HidHost::dpad_bindings_reverse[midiButtons.dpad];
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    switch (button)
    {
    case ProGuitarMidi_A:
        return midiButtons.a;
    case ProGuitarMidi_B:
        return midiButtons.b;
    case ProGuitarMidi_X:
        return midiButtons.x;
    case ProGuitarMidi_Y:
        return midiButtons.y;
    case ProGuitarMidi_Back:
        return midiButtons.back;
    case ProGuitarMidi_Start:
        return midiButtons.start;
    case ProGuitarMidi_Guide:
        return midiButtons.guide;
    case ProGuitarMidi_DpadUp:
        return up;
    case ProGuitarMidi_DpadDown:
        return down;
    case ProGuitarMidi_DpadLeft:
        return left;
    case ProGuitarMidi_DpadRight:
        return right;
        // map 5 fret frets based on pressed frets
    case ProGuitar_Green:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 1 || midiFrets[i] == 6 || midiFrets[i] == 13)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_Red:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 2 || midiFrets[i] == 7 || midiFrets[i] == 14)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_Yellow:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 3 || midiFrets[i] == 8 || midiFrets[i] == 15)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_Blue:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 4 || midiFrets[i] == 9 || midiFrets[i] == 16)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_Orange:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 5 || midiFrets[i] == 10 || midiFrets[i] == 17)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_SoloGreen:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 13)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_SoloRed:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 14)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_SoloYellow:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 15)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_SoloBlue:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 16)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_SoloOrange:
    {
        for (size_t i = 0; i < TU_ARRAY_SIZE(midiFrets); i++)
        {
            if (midiFrets[i] == 17)
            {
                return true;
            }
        }
        return false;
    }
    case ProGuitar_Pedal:
        // pro guitar just sends sustain pedal cc on chan 1
        return m_midi.read_midi_control_change(0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL) > (40 << 9);
    }
    return 0;
}
uint16_t MidiDevice::read_pro_guitar_axis(proto_ProGuitarAxisType axis)
{
    return m_midi.pro_guitar_axis(axis);
}


bool ProGuitarMidiDevice::read_pro_guitar_button(proto_ProGuitarMidiButtonType button)
{
    return m_midi_device->read_pro_guitar_button(button);
}
uint16_t ProGuitarMidiDevice::read_pro_guitar_axis(proto_ProGuitarAxisType axis)
{
    return m_midi_device->read_pro_guitar_axis(axis);
}