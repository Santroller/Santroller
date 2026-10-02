#include "devices/secondary_pico.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
#include <pb_decode.h>
#include <pb_encode.h>

static bool encode_inputs_callback(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    const auto *inputs = static_cast<const std::vector<proto_Input> *>(*arg);
    if (!inputs) return true;

    for (const auto &input : *inputs)
    {
        if (!pb_encode_tag_for_field(stream, field))
            return false;
        if (!pb_encode_submessage(stream, proto_Input_fields, &input))
            return false;
    }
    return true;
}

extern "C" {
    extern uint32_t __FLASH_APP_START;
    extern uint32_t __flash_binary_end;
}

SecondaryPicoDevice::SecondaryPicoDevice(proto_PeripheralDevice device, uint16_t id)
    : Device(id),
      interface(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock),
      m_device(device),
      address(device.address > 0 ? device.address : SECONDARY_PICO_BASE_ADDR)
{
}

void SecondaryPicoDevice::add_input(const proto_Input &input)
{
    m_inputs.push_back(input);
}

void SecondaryPicoDevice::begin()
{
    m_connected = false;
    m_last_poll = 0;
    m_last_recv = 0;
    m_digital_inputs.clear();
    m_analog_inputs.clear();
    m_version_checked = false;
    m_ota_in_progress = false;
    m_ota_offset = 0;
    memset(m_version_rx_buf, 0, sizeof(m_version_rx_buf));

    interface.dmaInit(address, this);
    send_config_to_slave();
}

void SecondaryPicoDevice::end(bool full)
{
    m_connected = false;
    m_version_checked = false;
    m_ota_in_progress = false;
    m_digital_inputs.clear();
    m_analog_inputs.clear();
    m_inputs.clear();
    interface.dmaDeinit(address);
}

void SecondaryPicoDevice::send_config_to_slave()
{
    uint8_t cfg_buf[512];
    cfg_buf[0] = SLAVE_CMD_CONFIG;

    proto_PeripheralDevice periph = m_device;
    periph.inputs.funcs.encode = &encode_inputs_callback;
    const void *arg = &m_inputs;
    periph.inputs.arg = const_cast<void*>(arg);

    pb_ostream_t stream = pb_ostream_from_buffer(cfg_buf + 1, sizeof(cfg_buf) - 1);
    if (pb_encode_delimited(&stream, proto_PeripheralDevice_fields, &periph))
    {
        interface.dmaWriteRead(address, cfg_buf, stream.bytes_written + 1, nullptr, 0);
    }
}

void SecondaryPicoDevice::update(bool full_poll, bool send_events)
{
    if (full_poll)
    {
        send_config_to_slave();
    }

    if (m_ota_in_progress)
    {
        if ((millis() - m_last_poll) < 5) return;
        m_last_poll = millis();

        const uint8_t *app_start = (const uint8_t*)&__FLASH_APP_START;
        const uint8_t *app_end = (const uint8_t*)&__flash_binary_end;
        uint32_t raw_fw_size = app_end > app_start ? static_cast<uint32_t>(app_end - app_start) : 0;

        if (m_ota_offset < m_ota_total_size)
        {
            uint8_t chunk[256];
            memset(chunk, 0, sizeof(chunk));
            if (m_ota_offset < raw_fw_size)
            {
                uint32_t copy_len = std::min<uint32_t>(256, raw_fw_size - m_ota_offset);
                memcpy(chunk, app_start + m_ota_offset, copy_len);
            }
            send_ota_chunk(m_ota_offset, chunk, 256);
            m_ota_offset += 256;

            uint32_t percent = m_ota_total_size > 0 ? (m_ota_offset * 100) / m_ota_total_size : 0;
            if (percent > 100) percent = 100;
            if (percent != m_last_ota_percent && (percent % 5 == 0 || m_ota_offset >= m_ota_total_size))
            {
                m_last_ota_percent = percent;
                proto_Event evt = proto_Event_init_zero;
                evt.which_event = proto_Event_device_tag;
                evt.event.device.id = m_id;
                evt.event.device.connected = true;
                evt.event.device.has_updating = true;
                evt.event.device.updating = true;
                evt.event.device.has_progress = true;
                evt.event.device.progress = percent;
                HIDConfigDevice::send_event(evt, true);
            }
        }
        else
        {
            send_ota_finish(m_ota_total_size);
            m_ota_in_progress = false;
            m_version_checked = false;
            m_rebooting = true;
            proto_Event evt = proto_Event_init_zero;
            evt.which_event = proto_Event_device_tag;
            evt.event.device.id = m_id;
            evt.event.device.connected = false;
            evt.event.device.has_updating = true;
            evt.event.device.updating = false;
            evt.event.device.has_progress = true;
            evt.event.device.progress = 100;
            evt.event.device.has_rebooting = true;
            evt.event.device.rebooting = true;
            HIDConfigDevice::send_event(evt, true);
        }
        return;
    }

    if ((millis() - m_last_poll) < 2) // Poll for streamed events every 2ms via DMA IRQ
        return;

    m_last_poll = millis();

    if (!m_version_checked)
    {
        m_version_cmd_buf[0] = SLAVE_CMD_GET_VERSION;
        interface.dmaWriteRead(address, m_version_cmd_buf, 1, m_version_rx_buf, 8);
        return;
    }

    tx_buf[0] = SLAVE_CMD_GET_EVENTS;
    interface.dmaWriteRead(address, tx_buf, 1, rx_buf, sizeof(rx_buf));
}

void SecondaryPicoDevice::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (timeout || abort_detected)
    {
        if (m_connected && (millis() - m_last_recv > 500))
        {
            m_connected = false;
            proto_Event event = proto_Event_init_zero;
            event.which_event = proto_Event_device_tag;
            event.event.device.id = m_id;
            event.event.device.connected = false;
            if (m_rebooting)
            {
                event.event.device.has_rebooting = true;
                event.event.device.rebooting = true;
            }
            HIDConfigDevice::send_event(event, true);
        }
        return;
    }

    m_last_recv = millis();
    if (!m_connected)
    {
        m_connected = true;
        proto_Event event = proto_Event_init_zero;
        event.which_event = proto_Event_device_tag;
        event.event.device.id = m_id;
        event.event.device.connected = true;
        HIDConfigDevice::send_event(event, true);
    }

    if (!m_version_checked)
    {
        m_version_checked = true;
        static const char main_ver[] = GIT_HASH;
        if (memcmp(m_version_rx_buf, main_ver, std::min<size_t>(8, strlen(main_ver))) != 0)
        {
            printf("Secondary Pico version mismatch! Starting auto-OTA...\n");
            const uint8_t *app_start = (const uint8_t*)&__FLASH_APP_START;
            const uint8_t *app_end = (const uint8_t*)&__flash_binary_end;
            uint32_t raw_fw_size = app_end > app_start ? static_cast<uint32_t>(app_end - app_start) : 0;
            if (raw_fw_size > 0 && raw_fw_size < 0x200000)
            {
                m_ota_total_size = ((raw_fw_size + 255) / 256) * 256;
                m_ota_offset = 0;
                m_ota_in_progress = true;
                m_last_ota_percent = 0;
                send_ota_begin(m_ota_total_size);

                proto_Event evt = proto_Event_init_zero;
                evt.which_event = proto_Event_device_tag;
                evt.event.device.id = m_id;
                evt.event.device.connected = true;
                evt.event.device.has_updating = true;
                evt.event.device.updating = true;
                evt.event.device.has_progress = true;
                evt.event.device.progress = 0;
                HIDConfigDevice::send_event(evt, true);
                return;
            }
        }
        else if (m_rebooting)
        {
            m_rebooting = false;
            proto_Event evt = proto_Event_init_zero;
            evt.which_event = proto_Event_device_tag;
            evt.event.device.id = m_id;
            evt.event.device.connected = true;
            evt.event.device.has_rebooting = true;
            evt.event.device.rebooting = false;
            HIDConfigDevice::send_event(evt, true);
        }
    }

    // Decode incoming streamed events from Secondary Pico
    pb_istream_t stream = pb_istream_from_buffer(rx_buf, sizeof(rx_buf));
    proto_Event event = proto_Event_init_default;
    while (stream.bytes_left > 0 && pb_decode_delimited(&stream, proto_Event_fields, &event))
    {
        if (event.which_event == proto_Event_button_tag)
        {
            uint32_t id = event.event.button.id;
            m_digital_inputs[id] = event.event.button.state;
        }
        else if (event.which_event == proto_Event_axis_tag)
        {
            uint32_t id = event.event.axis.id;
            m_analog_inputs[id] = static_cast<uint16_t>(event.event.axis.state);
        }
        event = proto_Event_init_default;
    }
}

bool SecondaryPicoDevice::read_digital_pin(uint32_t input_id)
{
    auto it = m_digital_inputs.find(input_id);
    if (it != m_digital_inputs.end()) return it->second;
    return false;
}

uint16_t SecondaryPicoDevice::read_analog_pin(uint32_t input_id)
{
    auto it = m_analog_inputs.find(input_id);
    if (it != m_analog_inputs.end()) return it->second;
    return 0;
}

bool SecondaryPicoDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.sda || pin == m_device.i2c.scl;
}

bool SecondaryPicoDevice::send_ota_begin(uint32_t total_size)
{
    uint8_t buf[5];
    buf[0] = SLAVE_CMD_OTA_BEGIN;
    memcpy(&buf[1], &total_size, 4);
    interface.dmaWriteRead(address, buf, 5, nullptr, 0);
    return true;
}

bool SecondaryPicoDevice::send_ota_chunk(uint32_t offset, const uint8_t *data, uint16_t length)
{
    uint8_t buf[261];
    if (length > 256) return false;
    buf[0] = SLAVE_CMD_OTA_CHUNK;
    memcpy(&buf[1], &offset, 4);
    memcpy(&buf[5], data, length);
    interface.dmaWriteRead(address, buf, 5 + length, nullptr, 0);
    return true;
}

bool SecondaryPicoDevice::send_ota_finish(uint32_t total_size)
{
    uint8_t buf[5];
    buf[0] = SLAVE_CMD_OTA_FINISH;
    memcpy(&buf[1], &total_size, 4);
    interface.dmaWriteRead(address, buf, 5, nullptr, 0);
    return true;
}
