#include "devices/midiserial.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "managers/profile_manager.hpp"
#include "config/config.hpp"
#include "pico/time.h"

// Sending the status byte again after this long idle lets a receiver plugged in mid stream pick up
#define MIDI_RUNNING_STATUS_TIMEOUT_US 250000

// Out only drives TX and In only listens on RX, whatever the other pin was left set to
MidiSerialDevice::MidiSerialDevice(const DeviceReloadState* state, proto_SerialMidiDevice device, uint16_t id) :
    MidiDevice(is_output(device) ? nullptr : state, id, false, m_midi_buffers.config()),
    serial(this, device.uart.block,
           is_output(device) ? device.uart.tx : -1,
           is_output(device) ? -1 : device.uart.rx,
           device.uart.baudrate),
    m_device(device)
{
}
bool MidiSerialDevice::matches_reload_config(const proto_Device &config) const
{
    if (config.which_device != proto_Device_midiSerial_tag)
    {
        return false;
    }
    const auto &device = config.device.midiSerial;
    return m_device.uart.block == device.uart.block &&
           m_device.uart.tx == device.uart.tx &&
           m_device.uart.rx == device.uart.rx &&
           m_device.uart.baudrate == device.uart.baudrate &&
           is_output() == is_output(device) &&
           m_device.has_noteHoldTime == device.has_noteHoldTime &&
           (!device.has_noteHoldTime || m_device.noteHoldTime == device.noteHoldTime);
}
void MidiSerialDevice::begin()
{
}

void MidiSerialDevice::end(bool full)
{
}
void MidiSerialDevice::update(bool full_poll, bool send_events)
{
    serial.tick();
    if (is_output())
    {
        send_output();
    }
    else
    {
        MidiDevice::update(full_poll, send_events);
    }
    if (m_lastConnected != serial.is_connected() || full_poll)
    {
        m_lastConnected = serial.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

// Plays whatever the MIDI profiles played last loop (devices update before the profiles do)
void MidiSerialDevice::send_output()
{
    uint64_t now = time_us_64();
    m_state.clear_all();
    ProfileManager::instance().for_each_instance([this](const std::shared_ptr<Instance> &instance)
                                                 {
                                                     const MidiState *state = instance->midi_output_state();
                                                     if (state)
                                                     {
                                                         m_state.merge(*state);
                                                     } });
    m_output.collect(m_state, now);

    if (now - m_last_tx_us > MIDI_RUNNING_STATUS_TIMEOUT_US)
    {
        m_running_status = 0;
    }
    while (serial.writable())
    {
        if (m_tx_pos == m_tx_len)
        {
            MidiMessage message;
            if (!m_output.pending(message))
            {
                break;
            }
            m_output.sent(message);
            // A note on with no velocity is a note off, and shares running status with the note ons
            if ((message.status & 0xF0) == MIDI_STATUS_NOTE_OFF)
            {
                message = {(uint8_t)(MIDI_STATUS_NOTE_ON | (message.status & 0x0F)), message.data1, 0};
            }
            m_tx_len = 0;
            m_tx_pos = 0;
            if (message.status != m_running_status)
            {
                m_tx[m_tx_len++] = message.status;
                m_running_status = message.status;
            }
            m_tx[m_tx_len++] = message.data1;
            m_tx[m_tx_len++] = message.data2;
        }
        serial.put(m_tx[m_tx_pos++]);
        m_last_tx_us = now;
    }
}

bool MidiSerialDevice::using_pin(uint8_t pin)
{
    return is_output() ? pin == m_device.uart.tx : pin == m_device.uart.rx;
}
