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

// A slave that stops answering is reported disconnected after this long
static constexpr uint32_t disconnect_timeout_ms = 500;
// An auto-OTA is abandoned (and restarted once the slave is back) after this long without progress
static constexpr uint32_t ota_timeout_ms = 2000;
static constexpr uint8_t host_transfer_attempts = 3;

static uint32_t firmware_size()
{
    const uint8_t *app_start = (const uint8_t *)&__FLASH_APP_START;
    const uint8_t *app_end = (const uint8_t *)&__flash_binary_end;
    return app_end > app_start ? static_cast<uint32_t>(app_end - app_start) : 0;
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
    m_host_ota = false;
    m_last_failed = false;
    m_config_dirty = true;
    m_transfer = Transfer::None;
    m_transfer_pending = false;
    m_transfer_done = false;
    memset(m_version_rx_buf, 0, sizeof(m_version_rx_buf));

    interface.dmaInit(address, this);
}

void SecondaryPicoDevice::end(bool full)
{
    m_connected = false;
    m_version_checked = false;
    m_ota_in_progress = false;
    m_host_ota = false;
    m_digital_inputs.clear();
    m_analog_inputs.clear();
    m_inputs.clear();
    interface.dmaDeinit(address);
    m_transfer = Transfer::None;
    m_transfer_pending = false;
}

// Called from the I2C IRQ, so only record the result; update() acts on it
void SecondaryPicoDevice::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    m_transfer_ok = stop_detected && !timeout && !abort_detected;
    m_transfer_done = true;
}

bool SecondaryPicoDevice::start_transfer(Transfer transfer, const uint8_t *wbuf, size_t wbuf_len, uint8_t *rbuf, size_t rbuf_len)
{
    m_transfer = transfer;
    m_transfer_done = false;
    m_transfer_pending = true;
    if (interface.dmaWriteRead(address, wbuf, wbuf_len, rbuf, rbuf_len, m_cmds))
        return true;
    m_transfer_pending = false;
    m_transfer = Transfer::None;
    return false;
}

void SecondaryPicoDevice::complete_transfer()
{
    Transfer transfer = m_transfer;
    m_transfer = Transfer::None;
    m_transfer_pending = false;
    if (m_transfer_ok)
        transfer_succeeded(transfer);
    else
        transfer_failed(transfer);
}

void SecondaryPicoDevice::wait_idle()
{
    while (m_transfer_pending)
    {
        if (m_transfer_done)
            complete_transfer();
        else
            tight_loop_contents();
    }
}

bool SecondaryPicoDevice::host_transfer(const uint8_t *wbuf, size_t wbuf_len)
{
    for (uint8_t attempt = 0; attempt < host_transfer_attempts; attempt++)
    {
        wait_idle();
        if (!start_transfer(Transfer::Host, wbuf, wbuf_len, nullptr, 0))
            return false;
        while (!m_transfer_done)
            tight_loop_contents();
        bool ok = m_transfer_ok;
        complete_transfer();
        if (ok)
            return true;
    }
    return false;
}

void SecondaryPicoDevice::send_ota_event(bool connected, bool updating, uint32_t progress, bool rebooting)
{
    proto_Event evt = proto_Event_init_zero;
    evt.which_event = proto_Event_device_tag;
    evt.event.device.id = m_id;
    evt.event.device.connected = connected;
    evt.event.device.has_updating = true;
    evt.event.device.updating = updating;
    evt.event.device.has_progress = true;
    evt.event.device.progress = progress;
    if (rebooting)
    {
        evt.event.device.has_rebooting = true;
        evt.event.device.rebooting = true;
    }
    HIDConfigDevice::send_event(evt, true);
}

void SecondaryPicoDevice::transfer_failed(Transfer transfer)
{
    m_last_failed = true;
    m_last_poll = millis();
    // Config and OTA steps are retried as they were never marked done
    if (transfer == Transfer::Config)
        m_config_dirty = true;

    if (m_ota_in_progress && (millis() - m_last_recv > ota_timeout_ms))
    {
        printf("Secondary Pico stopped responding during OTA\n");
        m_ota_in_progress = false;
        send_ota_event(false, false, m_last_ota_percent, false);
    }

    if (m_connected && (millis() - m_last_recv > disconnect_timeout_ms))
    {
        m_connected = false;
        // Whatever comes back may have rebooted, so check its version and send its config again
        m_version_checked = false;
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
}

void SecondaryPicoDevice::transfer_succeeded(Transfer transfer)
{
    m_last_failed = false;
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

    switch (transfer)
    {
    case Transfer::Version:
        version_received();
        break;
    case Transfer::Events:
        decode_events();
        break;
    case Transfer::OtaBegin:
        m_ota_stage = OtaStage::Chunks;
        break;
    case Transfer::OtaChunk:
    {
        m_ota_offset += SECONDARY_PICO_OTA_CHUNK_SIZE;
        uint32_t percent = m_ota_total_size > 0 ? (m_ota_offset * 100) / m_ota_total_size : 0;
        if (percent > 100)
            percent = 100;
        if (percent != m_last_ota_percent && (percent % 5 == 0 || m_ota_offset >= m_ota_total_size))
        {
            m_last_ota_percent = percent;
            send_ota_event(true, true, percent, false);
        }
        if (m_ota_offset >= m_ota_total_size)
            m_ota_stage = OtaStage::Finish;
        break;
    }
    case Transfer::OtaFinish:
        m_ota_in_progress = false;
        m_version_checked = false;
        m_rebooting = true;
        send_ota_event(false, false, 100, true);
        break;
    default:
        break;
    }
}

void SecondaryPicoDevice::version_received()
{
    m_version_checked = true;
    static const char main_ver[] = GIT_HASH;
    if (memcmp(m_version_rx_buf, main_ver, std::min<size_t>(8, strlen(main_ver))) != 0)
    {
        printf("Secondary Pico version mismatch! Starting auto-OTA...\n");
        uint32_t raw_fw_size = firmware_size();
        if (raw_fw_size > 0 && raw_fw_size < 0x200000)
        {
            m_ota_total_size = ((raw_fw_size + SECONDARY_PICO_OTA_CHUNK_SIZE - 1) / SECONDARY_PICO_OTA_CHUNK_SIZE) * SECONDARY_PICO_OTA_CHUNK_SIZE;
            m_ota_offset = 0;
            m_ota_stage = OtaStage::Begin;
            m_ota_in_progress = true;
            m_last_ota_percent = 0;
            send_ota_event(true, true, 0, false);
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
    // The slave only keeps its config until it reboots, and this runs after every (re)connect
    m_config_dirty = true;
}

void SecondaryPicoDevice::decode_events()
{
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

void SecondaryPicoDevice::send_config()
{
    m_config_buf[0] = SLAVE_CMD_CONFIG;

    proto_PeripheralDevice periph = m_device;
    periph.inputs.funcs.encode = &encode_inputs_callback;
    const void *arg = &m_inputs;
    periph.inputs.arg = const_cast<void*>(arg);

    pb_ostream_t stream = pb_ostream_from_buffer(m_config_buf + 1, sizeof(m_config_buf) - 1);
    if (!pb_encode_delimited(&stream, proto_PeripheralDevice_fields, &periph))
    {
        printf("Secondary Pico config does not fit in %u bytes\n", (unsigned)sizeof(m_config_buf));
        m_config_dirty = false;
        return;
    }
    // Cleared before it is sent so a full poll during the transfer queues another
    m_config_dirty = false;
    if (!start_transfer(Transfer::Config, m_config_buf, stream.bytes_written + 1, nullptr, 0))
        m_config_dirty = true;
}

void SecondaryPicoDevice::ota_step()
{
    switch (m_ota_stage)
    {
    case OtaStage::Begin:
        m_ota_cmd_buf[0] = SLAVE_CMD_OTA_BEGIN;
        memcpy(&m_ota_cmd_buf[1], &m_ota_total_size, 4);
        start_transfer(Transfer::OtaBegin, m_ota_cmd_buf, sizeof(m_ota_cmd_buf), nullptr, 0);
        break;
    case OtaStage::Chunks:
    {
        const uint8_t *app_start = (const uint8_t *)&__FLASH_APP_START;
        uint32_t raw_fw_size = firmware_size();
        uint8_t *chunk = &m_ota_chunk_buf[5];
        memset(chunk, 0, SECONDARY_PICO_OTA_CHUNK_SIZE);
        if (m_ota_offset < raw_fw_size)
        {
            uint32_t copy_len = std::min<uint32_t>(SECONDARY_PICO_OTA_CHUNK_SIZE, raw_fw_size - m_ota_offset);
            memcpy(chunk, app_start + m_ota_offset, copy_len);
        }
        m_ota_chunk_buf[0] = SLAVE_CMD_OTA_CHUNK;
        memcpy(&m_ota_chunk_buf[1], &m_ota_offset, 4);
        start_transfer(Transfer::OtaChunk, m_ota_chunk_buf, sizeof(m_ota_chunk_buf), nullptr, 0);
        break;
    }
    case OtaStage::Finish:
        m_ota_cmd_buf[0] = SLAVE_CMD_OTA_FINISH;
        memcpy(&m_ota_cmd_buf[1], &m_ota_total_size, 4);
        start_transfer(Transfer::OtaFinish, m_ota_cmd_buf, sizeof(m_ota_cmd_buf), nullptr, 0);
        break;
    }
}

void SecondaryPicoDevice::update(bool full_poll, bool send_events)
{
    if (full_poll)
        m_config_dirty = true;

    if (m_transfer_pending)
    {
        if (!m_transfer_done)
            return;
        complete_transfer();
    }

    // The configurator owns the bus until it finishes its OTA
    if (m_host_ota)
        return;

    uint32_t now = millis();
    // Back off after a failure rather than hammering a slave that isn't answering
    if (m_last_failed && (now - m_last_poll) < 2)
        return;

    if (m_ota_in_progress)
    {
        ota_step();
        return;
    }

    if (m_version_checked && m_config_dirty)
    {
        send_config();
        return;
    }

    if ((now - m_last_poll) < 2) // Poll for streamed events every 2ms
        return;
    m_last_poll = now;

    if (!m_version_checked)
    {
        m_version_cmd_buf[0] = SLAVE_CMD_GET_VERSION;
        start_transfer(Transfer::Version, m_version_cmd_buf, 1, m_version_rx_buf, 8);
        return;
    }

    tx_buf[0] = SLAVE_CMD_GET_EVENTS;
    start_transfer(Transfer::Events, tx_buf, 1, rx_buf, sizeof(rx_buf));
}

bool SecondaryPicoDevice::send_ota_begin(uint32_t total_size)
{
    // Waits for anything in flight first, as that may be using the same buffers
    wait_idle();
    m_ota_in_progress = false;
    m_host_ota = true;
    m_ota_cmd_buf[0] = SLAVE_CMD_OTA_BEGIN;
    memcpy(&m_ota_cmd_buf[1], &total_size, 4);
    return host_transfer(m_ota_cmd_buf, sizeof(m_ota_cmd_buf));
}

bool SecondaryPicoDevice::send_ota_chunk(uint32_t offset, const uint8_t *data, uint16_t length)
{
    if (length > SECONDARY_PICO_OTA_CHUNK_SIZE)
        return false;
    wait_idle();
    m_ota_chunk_buf[0] = SLAVE_CMD_OTA_CHUNK;
    memcpy(&m_ota_chunk_buf[1], &offset, 4);
    memcpy(&m_ota_chunk_buf[5], data, length);
    return host_transfer(m_ota_chunk_buf, 5 + length);
}

bool SecondaryPicoDevice::send_ota_finish(uint32_t total_size)
{
    wait_idle();
    m_ota_cmd_buf[0] = SLAVE_CMD_OTA_FINISH;
    memcpy(&m_ota_cmd_buf[1], &total_size, 4);
    bool ok = host_transfer(m_ota_cmd_buf, sizeof(m_ota_cmd_buf));
    m_host_ota = false;
    m_version_checked = false;
    m_rebooting = true;
    return ok;
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
