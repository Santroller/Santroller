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
#include <memory>

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
// Clock a PS1 drives its pads at, used until a pad identifies as a DualShock 2
#define PS1_CLOCK 250000
// Pads that can share one bus, each with its own attention pin
#define PSX_MAX_PORTS 4
// Idle bus time between one port releasing the bus and the next port taking it (us)
#define BUS_HANDOFF_DELAY 10

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

class PSXController;

/** \brief SPI bus shared by every pad wired to the same CLK, CMD, DAT and ACK lines
 *
 * Owns the SPI block, the DMA channels and the ACK pacer. Pads are told apart
 * only by their attention pin, so one port holds the bus from pulling its
 * attention low until it releases it again.
 */
class PSXBus
{
public:
    // Returns the bus for this SPI block, or nullptr if it is already in use with different pins
    static std::shared_ptr<PSXBus> acquire(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint8_t ackPin, uint32_t clock);
    static bool is_attached(const PSXController *port);
    static void dma_complete();
    static void ack_edge(uint gpio);
    ~PSXBus();
    bool attach(PSXController *port, uint8_t attPin);
    void detach(PSXController *port);
    bool claim(PSXController *port);
    void release(PSXController *port);
    bool is_active(const PSXController *port) const { return m_active == port; }
    void set_clock(uint32_t clock);

    spi_inst_t *spi = nullptr;
    int dma_rx = -1;
    int dma_tx = -1;
    int dma_tx_pacer = -1;
    int dma_timer = -1;
    PIO pio = nullptr;
    uint sm = 0;
    uint pio_offset = 0;
    bool pio_initialized = false;

private:
    PSXBus(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint8_t ackPin);
    void init(uint32_t clock);
    void stop_transfer();
    void update_timer_pacing();
    uint8_t m_block;
    int8_t m_sckPin;
    int8_t m_mosiPin;
    int8_t m_misoPin;
    uint8_t m_ackPin;
    uint32_t m_clock = 0;
    PSXController *m_ports[PSX_MAX_PORTS] = {};
    uint8_t m_attPins[PSX_MAX_PORTS] = {};
    bool m_pending[PSX_MAX_PORTS] = {};
    // Port mid transaction, or the port the bus was just handed to
    PSXController *m_active = nullptr;
    // Port that released the bus last, which gets ACK edges seen while the bus is idle
    PSXController *m_last = nullptr;
};

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
    void bus_granted();

private:
    bool attach_bus();
    bool auto_shift_data(const uint8_t *out, const uint8_t len);
    void no_attention();
    void signal_attention();
    uint32_t handshake_clock() const;
    void set_bus_clock(uint32_t clock);
    std::shared_ptr<PSXBus> m_bus;
    bool m_begun = false;
    // Clock this pad wants, applied whenever it takes the bus
    uint32_t m_bus_clock = PS1_CLOCK;
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
    // Cached from the bus, which owns them
    spi_inst_t *spi = nullptr;
    int dma_rx = -1;
    int dma_tx = -1;
    int dma_tx_pacer = -1;
    int dma_timer = -1;
    PIO pio = nullptr;
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
    uint8_t m_config_retries = 0;
    uint8_t m_enter_config_attempts = 0;
};