#include "midi.hpp"
#include "config.h"
MidiDevice::MidiDevice(bool usbBased, Midi_Data_t* data) : usbBased(usbBased), midiData(data)
{
    tu_memclr(&ep_stream, sizeof(ep_stream));
    tu_edpt_stream_init(&ep_stream.rx, true, false, false,
                        ep_stream.rx_ff_buf, 512, m_ep_in_buf);
    tu_edpt_stream_init(&ep_stream.tx, true, true, false,
                        ep_stream.tx_ff_buf, 512, m_ep_out_buf);
    memset(cable_status, 0, sizeof(cable_status));
}

MidiDevice::~MidiDevice()
{
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
                // proto_Event event = {which_event : proto_Event_midiDebug_tag, event : {midiDebug : {data : {size : data_size, bytes : {0}}}}};
                // memcpy(event.event.midiDebug.data.bytes, cable_state->data, data_size);
                // HIDConfigDevice::send_event(event, false);
            }
            uint8_t status = (cable_state->data[0] & 0xf0) >> 4;
            uint8_t channel = cable_state->data[0] & 0x0f;
            switch (status)
            {
            case MIDI_CIN_NOTE_OFF:
                // ignore note off since some drums don't send it, or they sent it too quick so we make our own note off later
                #if DEVICE_TYPE_IS_DRUM
                    break;
                #endif
                // velocity 0 for note off
                cable_state->data[2] = 0;
                [[fallthrough]];
            case MIDI_CIN_NOTE_ON:
                midiData->midiVelocities[cable_state->data[1]] = cable_state->data[2];
                break;
            case MIDI_CIN_CONTROL_CHANGE:
                midiData->midiControlChanges[cable_state->data[1]] = cable_state->data[2];
                break;
            case MIDI_CIN_PITCH_BEND_CHANGE:
                midiData->midiPitchWheel = ((int16_t)cable_state->data[3] << 7) | cable_state->data[2];
                break;
            case MIDI_CIN_POLY_KEYPRESS:
                break;
            case MIDI_CIN_PROGRAM_CHANGE:
            case MIDI_CIN_CHANNEL_PRESSURE:
                break;
            default:
                break;
            }
            if (cable_state->data[0] < MIDI_STATUS_SYSEX_START)
            {
                // if (seenChannels.find({m_id, channel}) == seenChannels.end())
                // {
                //     if (HIDConfigDevice::tool_closed())
                //     {
                //         printf("Seen new MIDI channel: %d on device %d\r\n", channel, m_id);
                //         seenChannels.insert_or_assign({m_id, channel}, true);
                //         reload();
                //     }
                // }
            }
            if (cable_state->data[0] == MIDI_STATUS_SYSEX_START)
            {
                uint8_t buttons_header[] = {MIDI_STATUS_SYSEX_START, 0x08, 0x40};
                if (memcmp(cable_state->data, buttons_header, sizeof(buttons_header)) == 0)
                {
                    // if (seenChannels.find({m_id, MIDI_CHANNEL_PROGUITAR}) == seenChannels.end())
                    // {
                    //     printf("Seen new MIDI channel: proguitar on device %d\r\n", m_id);
                    //     seenChannels[{m_id, MIDI_CHANNEL_PROGUITAR}] = true;
                    //     if (HIDConfigDevice::tool_closed())
                    //     {
                    //         reload();
                    //     }
                    // }
                    midiData->seenProGuitar = true;
                    if (cable_state->data[4] == 0x08)
                    {
                        // button events
                        memcpy(&midiData->proGuitarData, cable_state->data, sizeof(midiData->proGuitarData));
                    }
                    if (cable_state->data[4] == 0x01)
                    {
                        // fret state events
                        uint8_t string = cable_state->data[5] - 1;
                        switch (string)
                        {
                        case 0:
                            midiData->midiFrets[string] = cable_state->data[6] - 0x40;
                            break;
                        case 1:
                            midiData->midiFrets[string] = cable_state->data[6] - 0x3B;
                            break;
                        case 2:
                            midiData->midiFrets[string] = cable_state->data[6] - 0x37;
                            break;
                        case 3:
                            midiData->midiFrets[string] = cable_state->data[6] - 0x32;
                            break;
                        case 4:
                            midiData->midiFrets[string] = cable_state->data[6] - 0x2D;
                            break;
                        case 5:
                            midiData->midiFrets[string] = cable_state->data[6] - 0x28;
                            break;
                        default:
                            break;
                        }
                    }
                    if (cable_state->data[4] == 0x05)
                    {
                        // picking events
                        uint8_t string = cable_state->data[5] - 1;
                        uint8_t velocity = cable_state->data[6];
                        midiData->midiStringVelocities[string] = velocity;
                    }
                }
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