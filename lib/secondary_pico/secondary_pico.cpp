#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/i2c_slave.h"
#include "pico_fota_bootloader/core.h"
#include "events.pb.h"
#include "device.pb.h"
#include "input.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>
#include "secondary_pico.hpp"
#include "config/device_factory.hpp"
#include "config/input_factory.hpp"
#include "devices/base.hpp"
#include "input/input.hpp"

#define SECONDARY_PICO_BASE_ADDR 0x75

#define SLAVE_CMD_CONFIG              0x01
#define SLAVE_CMD_GET_EVENTS          0x02
#define SLAVE_CMD_GET_VERSION         0x03
#define SLAVE_CMD_INITIALISE          0x0D
#define SLAVE_CMD_OTA_BEGIN           0x80
#define SLAVE_CMD_OTA_CHUNK           0x81
#define SLAVE_CMD_OTA_FINISH          0x82

#define EVENT_QUEUE_MAX 32

static proto_Event g_event_queue[EVENT_QUEUE_MAX];
static uint8_t g_event_head = 0;
static uint8_t g_event_tail = 0;

static uint8_t g_ota_buffer[256];
static uint32_t g_ota_expected_size = 0;

static bool g_is_secondary_pico_mode = false;
static i2c_inst_t *g_slave_i2c = nullptr;
static uint g_slave_sda = 0;
static uint g_slave_scl = 0;
static uint g_slave_id_pin = 0;

static uint8_t g_rx_buf[512];
static size_t g_rx_len = 0;
static uint8_t g_tx_buf[512];
static size_t g_tx_len = 0;
static size_t g_tx_ptr = 0;

static uint8_t g_pending_config_buf[512];
static size_t g_pending_config_len = 0;
static volatile bool g_config_pending = false;

static std::vector<std::shared_ptr<Device>> g_secondary_sub_devices;
static std::vector<std::unique_ptr<Input>> g_secondary_inputs;
static std::unordered_map<uint32_t, bool> g_last_input_buttons;
static std::unordered_map<uint32_t, uint16_t> g_last_input_axes;

bool is_secondary_pico_mode()
{
    return g_is_secondary_pico_mode;
}

i2c_inst_t* get_secondary_pico_slave_i2c()
{
    return g_slave_i2c;
}

uint get_secondary_pico_slave_sda()
{
    return g_slave_sda;
}

uint get_secondary_pico_slave_scl()
{
    return g_slave_scl;
}

uint get_secondary_pico_slave_id_pin()
{
    return g_slave_id_pin;
}

bool secondary_pico_enqueue_event(const proto_Event *event)
{
    uint8_t next_head = (g_event_head + 1) % EVENT_QUEUE_MAX;
    if (next_head == g_event_tail)
        return false; // Queue full
    g_event_queue[g_event_head] = *event;
    g_event_head = next_head;
    return true;
}

static bool secondary_pico_dequeue_event(proto_Event *event)
{
    if (g_event_head == g_event_tail)
        return false; // Queue empty
    *event = g_event_queue[g_event_tail];
    g_event_tail = (g_event_tail + 1) % EVENT_QUEUE_MAX;
    return true;
}

size_t secondary_pico_handle_get_events(uint8_t *tx_buf, size_t max_len)
{
    pb_ostream_t stream = pb_ostream_from_buffer(tx_buf, max_len);
    proto_Event event;
    while (secondary_pico_dequeue_event(&event))
    {
        if (!pb_encode_delimited(&stream, proto_Event_fields, &event))
            break;
    }
    return stream.bytes_written;
}

void secondary_pico_handle_ota_command(const uint8_t *rx_buf, size_t rx_len)
{
    if (rx_len < 1) return;
    uint8_t cmd = rx_buf[0];

    if (cmd == SLAVE_CMD_OTA_BEGIN && rx_len >= 5)
    {
        memcpy(&g_ota_expected_size, &rx_buf[1], 4);
        pfb_initialize_download_slot();
    }
    else if (cmd == SLAVE_CMD_OTA_CHUNK && rx_len >= 5)
    {
        uint32_t offset = 0;
        memcpy(&offset, &rx_buf[1], 4);
        size_t chunk_len = rx_len - 5;
        if (chunk_len <= sizeof(g_ota_buffer))
        {
            memcpy(g_ota_buffer, &rx_buf[5], chunk_len);
            pfb_write_to_flash_aligned_256_bytes(g_ota_buffer, offset, chunk_len);
        }
    }
    else if (cmd == SLAVE_CMD_OTA_FINISH && rx_len >= 5)
    {
        uint32_t total_size = 0;
        memcpy(&total_size, &rx_buf[1], 4);
        if (pfb_firmware_sha256_check(total_size) == 0)
        {
            pfb_mark_download_slot_as_valid();
            pfb_perform_update();
        }
    }
}

static bool decode_sub_device_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Device dev = proto_Device_init_zero;
    if (!pb_decode(stream, proto_Device_fields, &dev))
    {
        return false;
    }
    uint16_t dev_id = static_cast<uint16_t>(g_secondary_sub_devices.size() + 1);
    auto sub_dev = DeviceFactory::create_device(dev, dev_id, nullptr);
    if (sub_dev)
    {
        sub_dev->begin();
        g_secondary_sub_devices.push_back(sub_dev);
    }
    return true;
}

static bool decode_sub_input_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Input in = proto_Input_init_zero;
    if (!pb_decode(stream, proto_Input_fields, &in))
    {
        return false;
    }
    auto input = InputFactory::create_input(in, nullptr);
    if (input)
    {
        g_secondary_inputs.push_back(std::move(input));
    }
    return true;
}

static void secondary_pico_handle_config_command(const uint8_t *rx_buf, size_t rx_len)
{
    for (auto &dev : g_secondary_sub_devices)
    {
        if (dev)
        {
            dev->end(true);
        }
    }
    g_secondary_sub_devices.clear();
    g_secondary_inputs.clear();
    g_last_input_buttons.clear();
    g_last_input_axes.clear();

    if (rx_len <= 1) return;

    proto_PeripheralDevice peripheral_config = proto_PeripheralDevice_init_zero;
    peripheral_config.devices.funcs.decode = &decode_sub_device_callback;
    peripheral_config.inputs.funcs.decode = &decode_sub_input_callback;

    pb_istream_t stream = pb_istream_from_buffer(rx_buf + 1, rx_len - 1);
    pb_decode_delimited(&stream, proto_PeripheralDevice_fields, &peripheral_config);
}

static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    switch (event)
    {
    case I2C_SLAVE_RECEIVE:
        if (g_rx_len < sizeof(g_rx_buf))
        {
            g_rx_buf[g_rx_len++] = i2c_read_byte_raw(i2c);
        }
        else
        {
            (void)i2c_read_byte_raw(i2c);
        }
        break;

    case I2C_SLAVE_REQUEST:
        if (g_tx_len == 0)
        {
            if (g_rx_len > 0 && g_rx_buf[0] == SLAVE_CMD_GET_EVENTS)
            {
                g_tx_len = secondary_pico_handle_get_events(g_tx_buf, sizeof(g_tx_buf));
            }
            else if (g_rx_len > 0 && g_rx_buf[0] == SLAVE_CMD_GET_VERSION)
            {
                static const char version_hash[] = GIT_HASH;
                size_t len = strlen(version_hash);
                if (len > sizeof(g_tx_buf)) len = sizeof(g_tx_buf);
                memcpy(g_tx_buf, version_hash, len);
                g_tx_len = len;
            }
            g_tx_ptr = 0;
        }
        if (g_tx_ptr < g_tx_len)
        {
            i2c_write_byte_raw(i2c, g_tx_buf[g_tx_ptr++]);
        }
        else
        {
            i2c_write_byte_raw(i2c, 0);
        }
        break;

    case I2C_SLAVE_FINISH:
        if (g_rx_len > 0)
        {
            if (g_rx_buf[0] & 0x80)
            {
                secondary_pico_handle_ota_command(g_rx_buf, g_rx_len);
            }
            else if (g_rx_buf[0] == SLAVE_CMD_CONFIG)
            {
                if (g_rx_len <= sizeof(g_pending_config_buf))
                {
                    memcpy(g_pending_config_buf, g_rx_buf, g_rx_len);
                    g_pending_config_len = g_rx_len;
                    g_config_pending = true;
                }
            }
        }
        g_rx_len = 0;
        g_tx_len = 0;
        g_tx_ptr = 0;
        break;
    }
}

void secondary_pico_slave_init(i2c_inst_t *i2c_block, uint sda_pin, uint scl_pin, uint id_gpio)
{
    g_slave_i2c = i2c_block;
    g_slave_sda = sda_pin;
    g_slave_scl = scl_pin;
    g_slave_id_pin = id_gpio;

    gpio_init(id_gpio);
    gpio_set_dir(id_gpio, false);
    gpio_set_pulls(id_gpio, true, false); // Pull-up

    sleep_ms(2);
    bool id_bit = gpio_get(id_gpio);
    uint8_t slave_addr = SECONDARY_PICO_BASE_ADDR + (id_bit ? 0 : 1);

    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    i2c_init(i2c_block, 400000);
    i2c_slave_init(i2c_block, slave_addr, i2c_slave_handler);
    g_is_secondary_pico_mode = true;
}

void secondary_pico_coprocessor_loop()
{
    uint32_t last_digital = sio_hw->gpio_in;
    uint16_t last_adc[4] = {0};

    adc_init();

    while (1)
    {
        if (g_config_pending)
        {
            g_config_pending = false;
            secondary_pico_handle_config_command(g_pending_config_buf, g_pending_config_len);
        }

        // 1. Direct GPIO pin fallback polling
        uint32_t current_digital = sio_hw->gpio_in;
        uint32_t diff = current_digital ^ last_digital;
        if (diff)
        {
            for (uint8_t pin = 0; pin < 30; pin++)
            {
                if (pin == g_slave_sda || pin == g_slave_scl || pin == g_slave_id_pin)
                    continue;

                if (diff & (1u << pin))
                {
                    bool state = (current_digital & (1u << pin)) != 0;
                    proto_Event event = proto_Event_init_zero;
                    event.which_event = proto_Event_button_tag;
                    event.event.button.id = pin;
                    event.event.button.state = state;
                    secondary_pico_enqueue_event(&event);
                }
            }
            last_digital = current_digital;
        }

        // 2. Direct ADC channel fallback polling
        for (uint8_t adc_channel = 0; adc_channel < 4; adc_channel++)
        {
            uint8_t gpio_pin = 26 + adc_channel;
            if (gpio_pin == g_slave_sda || gpio_pin == g_slave_scl || gpio_pin == g_slave_id_pin)
                continue;

            adc_select_input(adc_channel);
            uint16_t raw = adc_read();
            uint16_t val16 = (raw << 4) | (raw >> 8);
            if (abs((int)val16 - (int)last_adc[adc_channel]) > 256)
            {
                last_adc[adc_channel] = val16;
                proto_Event event = proto_Event_init_zero;
                event.which_event = proto_Event_axis_tag;
                event.event.axis.id = adc_channel;
                event.event.axis.state = val16;
                secondary_pico_enqueue_event(&event);
            }
        }

        // 3. Update active sub-devices
        for (auto &dev : g_secondary_sub_devices)
        {
            if (dev)
            {
                dev->update(false, true);
            }
        }

        // 4. Tick inputs configured on coprocessor
        for (size_t i = 0; i < g_secondary_inputs.size(); i++)
        {
            auto &input = g_secondary_inputs[i];
            if (!input) continue;

            uint32_t input_id = static_cast<uint32_t>(i + 1);
            if (input->has_independent_analog_value())
            {
                uint16_t current_val = input->tick_analog();
                auto it = g_last_input_axes.find(input_id);
                uint16_t last_val = (it != g_last_input_axes.end()) ? it->second : 0;
                if (abs((int)current_val - (int)last_val) > 256)
                {
                    g_last_input_axes[input_id] = current_val;
                    proto_Event event = proto_Event_init_zero;
                    event.which_event = proto_Event_axis_tag;
                    event.event.axis.id = input_id;
                    event.event.axis.state = current_val;
                    event.event.axis.stateRaw = current_val;
                    secondary_pico_enqueue_event(&event);
                }
            }
            else
            {
                bool current_state = input->tick_digital();
                auto it = g_last_input_buttons.find(input_id);
                bool last_state = (it != g_last_input_buttons.end()) ? it->second : false;
                if (current_state != last_state)
                {
                    g_last_input_buttons[input_id] = current_state;
                    proto_Event event = proto_Event_init_zero;
                    event.which_event = proto_Event_button_tag;
                    event.event.button.id = input_id;
                    event.event.button.state = current_state;
                    event.event.button.stateRaw = current_state;
                    secondary_pico_enqueue_event(&event);
                }
            }
        }

        tight_loop_contents();
    }
}

uint32_t secondary_pico_get_digital_inputs()
{
    return sio_hw->gpio_in;
}