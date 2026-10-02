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
#define SLAVE_CMD_INITIALISE          0x0D
#define SLAVE_CMD_OTA_BEGIN           0x80
#define SLAVE_CMD_OTA_CHUNK           0x81
#define SLAVE_CMD_OTA_FINISH          0x82

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
    void send_config_to_slave();
    void add_input(const proto_Input &input);

    bool send_ota_begin(uint32_t total_size);
    bool send_ota_chunk(uint32_t offset, const uint8_t *data, uint16_t length);
    bool send_ota_finish(uint32_t total_size);

    inline bool is_connected() const { return m_connected; }

private:
    I2CMasterInterface interface;
    proto_PeripheralDevice m_device;
    uint8_t address;
    bool m_connected = false;

    std::vector<proto_Input> m_inputs;
    std::unordered_map<uint32_t, bool> m_digital_inputs;
    std::unordered_map<uint32_t, uint16_t> m_analog_inputs;

    uint8_t tx_buf[8];
    uint8_t rx_buf[256];
    uint32_t m_last_poll = 0;
    uint32_t m_last_recv = 0;
};
