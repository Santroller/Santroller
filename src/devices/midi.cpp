#include "devices/midi.hpp"
#include "events.pb.h"
#include "usb/device/hid_device.h"
#include "main.hpp"
#include "config.hpp"

static std::map<std::tuple<uint16_t, uint16_t>, bool> seenChannels;
MidiDevice::MidiDevice(uint16_t id, bool usbBased) : Device(id), drumMode(false), usbBased(usbBased)
{
    printf("MIDI Device created with id %d\r\n", id);
    tu_memclr(&ep_stream, sizeof(ep_stream));
    tu_edpt_stream_init(&ep_stream.rx, true, false, false,
                        ep_stream.rx_ff_buf, TUH_EPSIZE_BULK_MAX, m_ep_in_buf);
    tu_edpt_stream_init(&ep_stream.tx, true, true, false,
                        ep_stream.tx_ff_buf, TUH_EPSIZE_BULK_MAX, m_ep_out_buf);
    memset(cable_status, 0, sizeof(cable_status));
    memset(midiVelocities, 0, sizeof(midiVelocities));
    memset(midiPitchWheel, 0, sizeof(midiPitchWheel));
    memset(midiControlChanges, 0, sizeof(midiControlChanges));
}

void MidiDevice::rescan(bool first)
{
    if (first)
    {
        for (int i = 0; i < 16; i++)
        {
            if (seenChannels.find({m_id, i}) != seenChannels.end())
            {
                if (usbBased)
                {
                    // USB host has more than one interface, so we need to grab it from assignable
                    assignable_devices.push_back(std::make_shared<MidiDeviceWithChannel>(m_id, i, std::static_pointer_cast<MidiDevice>(assignable_devices.back())));
                }
                else
                {
                    // Every other device will do this scanning on creation, so we grab from active_devices
                    assignable_devices.push_back(std::make_shared<MidiDeviceWithChannel>(m_id, i, std::static_pointer_cast<MidiDevice>(active_devices.back())));
                }
                printf("Assigning MIDI channel: %d on device %d\r\n", i, m_id);
            }
        }
    }
}

MidiDevice::~MidiDevice()
{
    printf("MIDI Device destroyed\r\n");
    tu_edpt_stream_deinit(&ep_stream.rx);
    tu_edpt_stream_deinit(&ep_stream.tx);
}

void MidiDevice::processMidiData(uint8_t *data, uint16_t len)
{
    tu_edpt_stream_t *ep_str_rx = &ep_stream.rx;
    memcpy(ep_str_rx->ep_buf, data, len);
    tu_edpt_stream_read_xfer_complete(ep_str_rx, len);
}
void MidiDevice::update(bool full_poll, bool send_events)
{
    uint8_t one_byte;
    if (!tu_edpt_stream_peek(&ep_stream.rx, &one_byte))
    {
        return;
    }
    cable_state_t *cable_state = &cable_status[0];
    uint8_t usb_packet[4];
    while (tu_edpt_stream_peek(&ep_stream.rx, &one_byte))
    {
        if (!usb_pos && usbBased)
        {
            tu_fifo_peek_n(&ep_stream.rx.ff, usb_packet, 4);
            if (tu_mem_is_zero(usb_packet, 4))
            {
                // no data, just empty packet, so skip it
                tu_edpt_stream_read(&ep_stream.rx, usb_packet, 4);
                continue;
            }
            // read cable number
            uint8_t p_cable_num;
            tu_edpt_stream_read(&ep_stream.rx, &p_cable_num, 1);
            cable_state = &cable_status[(p_cable_num >> 4) & 0x0f];
            usb_pos++;
            continue;
        }
        else if (cable_state->pos == 0)
        {
            tu_edpt_stream_read(&ep_stream.rx, cable_state->data, 1);
            cable_state->pos++;
            uint8_t status = cable_state->data[0];
            if (status == MIDI_STATUS_SYSEX_START)
            {
                cable_state->sysex_in_progress = true;
                // we don't know the actual size of the message yet
                cable_state->actual_size = 0xFF;
            }
            if (status < MIDI_STATUS_SYSEX_START)
            {
                // then it is a channel message either three bytes or two
                uint8_t fake_cin = (status & 0xf0) >> 4;
                switch (fake_cin)
                {
                case MIDI_CIN_NOTE_OFF:
                case MIDI_CIN_NOTE_ON:
                    cable_state->actual_size = 3;
                    break;
                case MIDI_CIN_CONTROL_CHANGE:
                    cable_state->actual_size = 3;
                    break;
                case MIDI_CIN_PITCH_BEND_CHANGE:
                    cable_state->actual_size = 3;
                    break;
                case MIDI_CIN_POLY_KEYPRESS:
                    cable_state->actual_size = 3;
                    break;
                case MIDI_CIN_PROGRAM_CHANGE:
                case MIDI_CIN_CHANNEL_PRESSURE:
                    cable_state->actual_size = 2;
                    break;
                default:
                    break; // Should not get this
                }
                cable_state->sysex_in_progress = false;
            }
            else if (status < MIDI_STATUS_SYSREAL_TIMING_CLOCK)
            {
                switch (status)
                {
                case MIDI_STATUS_SYSCOM_TIME_CODE_QUARTER_FRAME:
                case MIDI_STATUS_SYSCOM_SONG_SELECT:
                    cable_state->actual_size = 2;
                    break;
                case MIDI_STATUS_SYSCOM_SONG_POSITION_POINTER:
                    cable_state->actual_size = 3;
                    break;
                case MIDI_STATUS_SYSCOM_TUNE_REQUEST:
                case MIDI_STATUS_SYSEX_END:
                    cable_state->actual_size = 1;
                    break;
                default:
                    break;
                }
                cable_state->sysex_in_progress = false;
            }
            else
            {
                cable_state->actual_size = 1;
                cable_state->sysex_in_progress = false;
            }
        }
        else
        {
            uint8_t d;
            tu_edpt_stream_read(&ep_stream.rx, &d, 1);
            if (d <= MIDI_MAX_DATA_VAL)
            {
                if (cable_state->pos < sizeof(cable_state->data))
                {
                    cable_state->data[cable_state->pos] = d;
                }
                cable_state->pos++;
            }
            else if (d == MIDI_STATUS_SYSEX_END)
            {
                if (cable_state->pos < sizeof(cable_state->data))
                {
                    cable_state->data[cable_state->pos] = d;
                }
                cable_state->pos++;
                cable_state->sysex_in_progress = false;
                cable_state->actual_size = cable_state->pos;
            }
            else
            {
                // at this point this is probably a realtime message
                // interleaved in the middle of a sysex message
                // and we don't care about that right now
                // so we ignore it
            }
        }
        usb_pos++;
        if (cable_state->actual_size && cable_state->pos >= cable_state->actual_size)
        {
            // then this is the last byte of the message, so reset pos for next message
            cable_state->pos = 0;
            // lets not send timing clock signals to the monitor
            if (cable_state->data[0] != MIDI_STATUS_SYSREAL_TIMING_CLOCK)
            {
                uint8_t data_size = MIN(32, cable_state->actual_size);
                proto_Event event = {which_event : proto_Event_midiDebug_tag, event : {midiDebug : {data : {size : data_size, bytes : {0}}}}};
                memcpy(event.event.midiDebug.data.bytes, cable_state->data, data_size);
                HIDConfigDevice::send_event(event);
            }
            uint8_t status = (cable_state->data[0] & 0xf0) >> 4;
            uint8_t channel = cable_state->data[0] & 0x0f;
            switch (status)
            {
            case MIDI_CIN_NOTE_OFF:
                // ignore note off since some drums don't send it, or they sent it too quick so we make our own note off later
                if (drumMode)
                {
                    break;
                }
                // velocity 0 for note off
                cable_state->data[2] = 0;
                [[fallthrough]];
            case MIDI_CIN_NOTE_ON:
                midiVelocities[channel][cable_state->data[1]] = cable_state->data[2];
                break;
            case MIDI_CIN_CONTROL_CHANGE:
                midiControlChanges[channel][cable_state->data[1]] = cable_state->data[2];
                break;
            case MIDI_CIN_PITCH_BEND_CHANGE:
                midiPitchWheel[channel] = ((int16_t)cable_state->data[3] << 7) | cable_state->data[2];
                break;
            case MIDI_CIN_POLY_KEYPRESS:
                break;
            case MIDI_CIN_PROGRAM_CHANGE:
            case MIDI_CIN_CHANNEL_PRESSURE:
                break;
            default:
                break;
            }
            if (status < MIDI_STATUS_SYSEX_START)
            {
                if (seenChannels.find({m_id, channel}) == seenChannels.end())
                {
                    printf("Seen new MIDI channel: %d on device %d\r\n", channel, m_id);
                    seenChannels[{m_id, channel}] = true;
                    if (HIDConfigDevice::tool_closed())
                    {
                        reload();
                    }
                }
            }
            if (cable_state->data[0] == MIDI_STATUS_SYSEX_START)
            {
                // TODO: handle pro guitar
                // printf("Sysex message: ");
                // for (int i = 0; i < cable_state->actual_size; i++)
                // {
                //     printf("%02x, ", cable_state->data[i]);
                // }
                // printf("\r\n");
            }
            // at this point we can look at the state and process the message.
            if (usb_pos < USB_PACKET_SIZE && usbBased)
            {
                // discard the unused bytes in the USB packet if we haven't already read them
                tu_edpt_stream_read(&ep_stream.rx, cable_state->data, USB_PACKET_SIZE - usb_pos);
            }
            usb_pos = 0;
        }
        if (usb_pos >= USB_PACKET_SIZE)
        {
            // then this is the end of the USB packet, so reset usb_pos for next packet
            usb_pos = 0;
        }
    }
}
uint16_t MidiDevice::readMidiNote(uint8_t channel, uint8_t note)
{
    return midiVelocities[channel][note] << 9;
}
uint16_t MidiDevice::readMidiControlChange(uint8_t channel, uint8_t cc)
{
    return midiControlChanges[channel][cc] << 9;
}
int16_t MidiDevice::readMidiPitchBend(uint8_t channel)
{
    return midiPitchWheel[channel];
}

uint16_t MidiDeviceWithChannel::readMidiNote(uint8_t note)
{
    return m_midi_device->readMidiNote(m_channel, note);
}
uint16_t MidiDeviceWithChannel::readMidiControlChange(uint8_t cc)
{
    return m_midi_device->readMidiControlChange(m_channel, cc);
}
int16_t MidiDeviceWithChannel::readMidiPitchBend()
{
    return m_midi_device->readMidiPitchBend(m_channel);
}