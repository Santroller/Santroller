#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>

#define I2C_MAX_ADDR 127
#define I2C_MAX_TRANSFER_SIZE 32
// A transfer timeout of 1000ms will allow a 10000 bit transfer to complete
// successfully without timeouts at baudrates as low as 10000 baud.
#define I2C_TRANSFER_TIMEOUT_MS 10000
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
} i2c_dma_slot_t;

typedef struct i2c_dma_s
{
    i2c_inst_t *i2c;

    uint irq_num;
    irq_handler_t irq_handler;

    uint baudrate;
    uint sda_gpio;
    uint scl_gpio;
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
    volatile bool queuing;
    volatile bool processing;
    alarm_id_t timeout_alarm_id;
    // The I2CMasterInterface whose transfer is running, its buffers are in use until it finishes
    const void *volatile runningFor;

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