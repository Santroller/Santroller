#include "devices/midi.hpp"
#include "events.pb.h"
#include "usb/device/hid_device.h"

MidiDevice::MidiDevice(uint16_t id, bool usbBased) : Device(id), drumMode(false), usbBased(usbBased)
{
    tu_memclr(&ep_stream, sizeof(ep_stream));
    tu_edpt_stream_init(&ep_stream.rx, true, false, false,
                        ep_stream.rx_ff_buf, TUH_EPSIZE_BULK_MAX, m_ep_in_buf);
    tu_edpt_stream_init(&ep_stream.tx, true, true, false,
                        ep_stream.tx_ff_buf, TUH_EPSIZE_BULK_MAX, m_ep_out_buf);
    memset(cable_status, 0, sizeof(cable_status));
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

uint16_t MidiDevice::readMidiNote(uint8_t note)
{
    return 0;
}
uint16_t MidiDevice::readMidiControlChange(uint8_t cc)
{
    return 0;
}
int16_t MidiDevice::readMidiPitchBend()
{
    return 0;
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
            // if (cable_state->data[0] != 0xF8)
            // {
            //     printf("got message (%d %d): ", cable_state->actual_size, usb_pos);
            //     for (int i = 0; i < cable_state->actual_size; i++)
            //     {
            //         printf("%02x, ", cable_state->data[i]);
            //     }
            //     printf("\r\n");
            // }
            if (cable_state->data[0] != 0xF8)
            {
                uint8_t data_size = MIN(32, cable_state->actual_size);
                proto_Event event = {which_event : proto_Event_midiDebug_tag, event : {midiDebug : {data : {size : data_size, bytes : {0}}}}};
                memcpy(event.event.midiDebug.data.bytes, cable_state->data, data_size);
                HIDConfigDevice::send_event(event), data_size;
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
                printf("Note %s: %02x, velocity %02x\r\n", (status == MIDI_CIN_NOTE_ON) ? "ON" : "OFF", cable_state->data[1], cable_state->data[2]);
                break;
            case MIDI_CIN_CONTROL_CHANGE:
                if (cable_state->data[1] == MIDI_CONTROL_COMMAND_MOD_WHEEL)
                {
                    midiModWheel[channel] = cable_state->data[2];
                }
                else if (cable_state->data[1] == MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL)
                {
                    midiSustainPedal[channel] = cable_state->data[2];
                }
                printf("Control Change: %02x, value %02x\r\n", cable_state->data[1], cable_state->data[2]);
                break;
            case MIDI_CIN_PITCH_BEND_CHANGE:
                midiPitchWheel[channel] = ((int16_t)cable_state->data[3] << 7) | cable_state->data[2];
                printf("Pitch Bend: %04x\r\n", (uint16_t)midiPitchWheel[channel]);
                break;
            case MIDI_CIN_POLY_KEYPRESS:
                printf("Channel message 3 bytes %02x %02x %02x\r\n", cable_state->data[1], cable_state->data[2], cable_state->data[3]);
                break;
            case MIDI_CIN_PROGRAM_CHANGE:
            case MIDI_CIN_CHANNEL_PRESSURE:
                printf("Channel message 2 bytes %02x %02x %02x\r\n", cable_state->data[1], cable_state->data[2]);
                break;
            default:
                break; // Should not get this
            }
            if (cable_state->data[0] == MIDI_STATUS_SYSEX_START)
            {
                printf("Sysex message: ");
                for (int i = 0; i < cable_state->actual_size; i++)
                {
                    printf("%02x, ", cable_state->data[i]);
                }
                printf("\r\n");
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