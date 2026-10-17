#pragma once

#include "base.hpp"
#include "device.pb.h"
#include "input.pb.h"
#include "events.pb.h"
#include "i2c.hpp"
#include <unordered_map>
#include <vector>

#define SECONDARY_PICO_BASE_ADDR 0x75

#define SLAVE_CMD_CONFIG              0x01
#define SLAVE_CMD_GET_EVENTS          0x02
#define SLAVE_CMD_GET_VERSION         0x03
#define SLAVE_CMD_INITIALISE          0x0D
#define SLAVE_CMD_OTA_BEGIN           0x80
#define SLAVE_CMD_OTA_CHUNK           0x81
#define SLAVE_CMD_OTA_FINISH          0x82

#define SECONDARY_PICO_CONFIG_SIZE    512
#define SECONDARY_PICO_OTA_CHUNK_SIZE 256
// Largest single transfer: the config write
#define SECONDARY_PICO_MAX_TRANSFER   SECONDARY_PICO_CONFIG_SIZE

class SecondaryPicoDevice : public Device, public I2CDMAInterface
{
public:
    SecondaryPicoDevice(proto_PeripheralDevice device, uint16_t id);
    ~SecondaryPicoDevice() {}

    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected) override;

    bool read_digital_pin(uint32_t input_id);
    uint16_t read_analog_pin(uint32_t input_id);
    void add_input(const proto_Input &input);

    // Configurator driven OTA. These block until the transfer is done, as the host waits on each
    // report before sending the next, and pause polling until send_ota_finish
    bool send_ota_begin(uint32_t total_size);
    bool send_ota_chunk(uint32_t offset, const uint8_t *data, uint16_t length);
    bool send_ota_finish(uint32_t total_size);

    inline bool is_connected() const { return m_connected; }

private:
    // There is one waiting transfer per address, so only one is in flight at a time. process_data
    // runs in the I2C IRQ and only records the result, update() acts on it and starts the next
    enum class Transfer : uint8_t
    {
        None,
        Version,
        Events,
        Config,
        OtaBegin,
        OtaChunk,
        OtaFinish,
        Host,
    };
    enum class OtaStage : uint8_t
    {
        Begin,
        Chunks,
        Finish,
    };
    bool start_transfer(Transfer transfer, const uint8_t *wbuf, size_t wbuf_len, uint8_t *rbuf, size_t rbuf_len);
    void complete_transfer();
    void wait_idle();
    bool host_transfer(const uint8_t *wbuf, size_t wbuf_len);
    void transfer_failed(Transfer transfer);
    void transfer_succeeded(Transfer transfer);
    void send_config();
    void ota_step();
    void version_received();
    void decode_events();
    void send_ota_event(bool connected, bool updating, uint32_t progress, bool rebooting);

    I2CMasterInterface interface;
    proto_PeripheralDevice m_device;
    uint8_t address;
    bool m_connected = false;

    std::vector<proto_Input> m_inputs;
    std::unordered_map<uint32_t, bool> m_digital_inputs;
    std::unordered_map<uint32_t, uint16_t> m_analog_inputs;

    // dmaWriteRead keeps the buffer pointers until the transfer starts, so everything handed to it
    // lives here rather than on the stack. Transfers over I2C_MAX_TRANSFER_SIZE also need m_cmds
    uint16_t m_cmds[SECONDARY_PICO_MAX_TRANSFER];
    uint8_t m_config_buf[SECONDARY_PICO_CONFIG_SIZE];
    uint8_t m_ota_chunk_buf[5 + SECONDARY_PICO_OTA_CHUNK_SIZE];
    uint8_t m_ota_cmd_buf[5];
    uint8_t tx_buf[8];
    uint8_t rx_buf[256];
    uint8_t m_version_cmd_buf[1] = {SLAVE_CMD_GET_VERSION};
    uint8_t m_version_rx_buf[16] = {0};

    Transfer m_transfer = Transfer::None;
    volatile bool m_transfer_pending = false;
    volatile bool m_transfer_done = false;
    volatile bool m_transfer_ok = false;
    bool m_last_failed = false;
    bool m_config_dirty = false;
    bool m_host_ota = false;

    uint32_t m_last_poll = 0;
    uint32_t m_last_recv = 0;
    bool m_version_checked = false;
    bool m_ota_in_progress = false;
    OtaStage m_ota_stage = OtaStage::Begin;
    bool m_rebooting = false;
    uint32_t m_ota_offset = 0;
    uint32_t m_ota_total_size = 0;
    uint32_t m_last_ota_percent = 0;
};
