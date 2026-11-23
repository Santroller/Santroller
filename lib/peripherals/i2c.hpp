#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>

#define I2C_MAX_ADDR 127
#define I2C_MAX_TRANSFER_SIZE 32
// Transfers time out at twice how long they should take, so a device holding the bus only stalls the
// others on it briefly. Each byte is 9 clocks, plus however long the device stretches the clock for
// (the GH5 peripherals take up to 85us a byte), and the margin covers starting up and interrupt latency
#define I2C_TRANSFER_STRETCH_US_PER_BYTE 200
#define I2C_TRANSFER_TIMEOUT_MARGIN_US 20000
#define I2C_TAKE_MUTEX_TIMEOUT_MS 10000

class I2CDMAInterface
{
public:
    I2CDMAInterface() {};
    ~I2CDMAInterface() {};
    virtual void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected) = 0;
};
#define I2C_MAX_DEVICES 16

typedef struct
{
    uint8_t addr;
    const uint8_t* wbuf;
    size_t wbuf_len;
    uint8_t* rbuf;
    size_t rbuf_len;
    uint16_t* cmds;
} i2c_dma_transfer_t;

typedef struct
{
    uint8_t addr;
    bool in_use;
    I2CDMAInterface *dmaInterface;
    // The I2CMasterInterface that registered the slot, and the one that queued waitingTransfer. They
    // differ when a replacement device takes over an address before the old device is torn down
    const void *owner;
    const void *volatile queuedBy;
    volatile i2c_dma_transfer_t waitingTransfer;
    volatile bool hasWaitingTransfer;
    // The clock the device asked for. The bus runs at the slowest of its devices' clocks
    uint32_t clock;
} i2c_dma_slot_t;

typedef struct i2c_dma_s
{
    i2c_inst_t *i2c;

    uint irq_num;
    irq_handler_t irq_handler;

    // The clock the bus runs at, the slowest its devices asked for. Changing it is left to the next
    // transfer to apply (with the register values worked out here), as it would garble one running
    uint baudrate;
    volatile bool baud_pending;
    uint32_t fs_scl_hcnt;
    uint32_t fs_scl_lcnt;
    uint32_t fs_spklen;
    uint32_t sda_tx_hold;
    // The bus has one set of pins, the first device's. Others asking for different pins are refused
    uint sda_gpio;
    uint scl_gpio;
    bool pins_claimed;
    int tx_chan;
    int rx_chan;
    bool reading;
    bool writing;
    uint device_count;
    volatile int currentSlot;

    volatile bool stop_detected;
    volatile bool abort_detected;
    volatile bool timeout;
    volatile bool running;
    volatile bool processing;
    alarm_id_t timeout_alarm_id;
    // The I2CMasterInterface whose transfer is running, its buffers are in use until it finishes
    const void *volatile runningFor;
    // A device in the middle of a sequence that has to run back to back (eg a Wii extension's init) can
    // hold the bus, so only its transfers run until it lets go or claimUntilUs passes
    volatile bool claimed;
    volatile int claimedSlot;
    volatile uint32_t claimUntilUs;

    uint16_t data_cmds[I2C_MAX_TRANSFER_SIZE];
    i2c_dma_slot_t slots[I2C_MAX_DEVICES];
} i2c_dma_t;

class I2CMasterInterface
{
public:
    I2CMasterInterface(uint8_t block, int8_t sda, int8_t scl, uint32_t clock);
    ~I2CMasterInterface();
    void dmaInit(uint8_t addr, I2CDMAInterface *dmaInterface);
    void dmaDeinit(uint8_t addr);
    void tick();
    // Only this address's transfers run until releaseBus, or until max_ms passes so a device that never
    // lets go can't starve the rest of the bus. Takes effect once the running transfer finishes. Returns
    // false if another device holds the bus
    bool claimBus(uint8_t addr, uint32_t max_ms);
    void releaseBus(uint8_t addr);
    // Returns false if the transfer was not queued, in which case process_data will not be called for it
    bool dmaWriteRead(
        uint8_t addr,
        const uint8_t *wbuf,
        size_t wbuf_len,
        uint8_t *rbuf,
        size_t rbuf_len);
    // For transfers larger than I2C_MAX_TRANSFER_SIZE: cmds must hold wbuf_len + rbuf_len entries
    bool dmaWriteRead(
        uint8_t addr,
        const uint8_t *wbuf,
        size_t wbuf_len,
        uint8_t *rbuf,
        size_t rbuf_len,
        uint16_t *cmds);

private:
    i2c_dma_t *i2c_dma = nullptr;
    i2c_inst_t *i2c;
    i2c_inst_t *_hardwareBlocks[NUM_I2CS] = {i2c0, i2c1};
    uint8_t m_sda;
    uint8_t m_scl;
    uint32_t m_clock;
};

class I2CSlaveInterface
{
public:
    void init();
    void on_byte_received(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_received_end(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_requested(uint8_t (*handler)(uint8_t addr));
};