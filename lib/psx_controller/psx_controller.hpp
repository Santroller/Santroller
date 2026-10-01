#pragma once
#include <stdint.h>

#include "enums.pb.h"
#include "input_enums.pb.h"
#include "pico/time.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/dma.h>
#include <hardware/pio.h>
#include "psx_controller.pio.h"
#include "devices/base.hpp"

#ifndef PS2_DEBUG
#define PS2_DEBUG 0
#endif

#if PS2_DEBUG
#include <stdio.h>
#define PS2_PRINT(...) printf(__VA_ARGS__)
#else
#define PS2_PRINT(...) ((void)0)
#endif

/** \brief Size of internal communication buffer
 *
 * This can be sized after the longest command reply (which is 21 bytes for
 * 01 42 when in DualShock 2 mode), but we're better safe than sorry.
 */
#define BUFFER_SIZE 32
/** \brief Command Inter-Byte Delay (us)
 *
 * Commands are several bytes long. This is the time to wait between two
 * consecutive bytes.
 *
 * This should actually be done by watching the \a Acknowledge line, but we are
 * ignoring it at the moment.
 */
#define INTER_CMD_BYTE_DELAY 100
/** \brief Command timeout (ms)
 *
 * Commands are sent to the controller repeatedly, until they succeed or time
 * out. This is the length of that timeout.
 *
 * \sa COMMAND_RETRY_INTERVAL
 */
#define COMMAND_TIMEOUT 250

/** \brief Command Retry Interval (ms)
 *
 * When sending a command to the controller, if it does not succeed, it is
 * retried after this amount of time.
 */
#define COMMAND_RETRY_INTERVAL 10

/** \brief Attention Delay
 *
 * Time between attention being issued to the controller and the first clock
 * edge (us).
 */
#define ATTN_DELAY 100

typedef enum 
{
    DISCONNECTED,
    CONNECTION_DELAY,
    FIRST_INPUTS,
    ENTER_CONFIG,
    ENABLE_ANALOG_MODE,
    ENABLE_RUMBLE,
    ENABLE_PRESSURES,
    ENABLE_PRESSURES_2,
    EXIT_CONFIG,
    SECOND_INPUTS,
    ENUMERATED
} PSXControllerState;

class PSXController
{
public:
    PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin);
    ~PSXController();
    void begin();
    void end();
    void load_state(const DeviceReloadState *state);
    void save_state(DeviceReloadState& state) const ;
    void tick();
    void set_rumble(uint8_t left, uint8_t right);
    PS2ControllerType type = PS2ControllerTypeUnknown;
    uint16_t read_axis(PS2AxisType type);
    bool read_button(PS2ButtonType type);
    bool controller_valid();
    void process_data(bool ack, bool timeout);
    void spi_dma_complete();
    void spi_header_complete();

private:
    bool auto_shift_data(const uint8_t *out, const uint8_t len);
    void no_attention();
    void signal_attention();
    spi_inst_t *spi = nullptr;
    uint8_t m_block;
    uint32_t m_clock;
    uint8_t m_attPin;
    uint8_t m_ackPin;
    int8_t m_sckPin;
    int8_t m_mosiPin;
    int8_t m_misoPin;
    uint8_t m_rumble_small = 0;
    uint8_t m_rumble_large = 0;
    uint8_t m_poll_cmd[5] = {0x01, 0x42, 0x00, 0x00, 0x00};
    int missing = 0;
    bool valid = false;
    bool hasTapBar = false;
    long last = 0;
    long lastInit = 0;
    uint8_t invalidCount = 0;
    uint8_t ps2Data[BUFFER_SIZE];
    uint8_t ps2DataOutBuffer[BUFFER_SIZE];
    int dma_rx = -1;
    int dma_tx = -1;
    int dma_tx_pacer = -1;
    int dma_timer = -1;
    PIO pio = pio0;
    uint sm = 0;
    uint pio_offset = 0;
    bool pio_initialized = false;
    bool spi_active = false;
    bool spi_started = false;
    bool spi_header = false;
    uint8_t spi_dma_offset = 0;
    uint8_t spi_dma_len = 0;
    uint8_t lastInputs[BUFFER_SIZE];
    const uint8_t* ps2DataOut;
    uint8_t ps2Idx;
    uint8_t ps2Len;
    uint8_t ps2DataLen;
    alarm_id_t timeout_alarm_id = 0;
    bool done = false;
    PSXControllerState status = DISCONNECTED;
    uint32_t packet_delay = 10000;
};