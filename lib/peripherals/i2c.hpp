#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>

#define I2C_MAX_TRANSFER_SIZE 1056
// A transfer timeout of 1000ms will allow a 10000 bit transfer to complete
// successfully without timeouts at baudrates as low as 10000 baud.
#define I2C_TRANSFER_TIMEOUT_MS 10000
#define I2C_TAKE_MUTEX_TIMEOUT_MS 10000
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

    volatile bool stop_detected;
    volatile bool abort_detected;
    volatile bool timeout;
    volatile bool running;
    alarm_id_t timeout_alarm_id;

    uint16_t data_cmds[I2C_MAX_TRANSFER_SIZE];
    void (*process_data)(bool running, bool timeout, bool abort_detected, bool stop_detected);
} i2c_dma_t;

class I2CMasterInterface
{
public:
    I2CMasterInterface(uint8_t block, int8_t sda, int8_t scl, uint32_t clock, void (*process_data)(bool running, bool timeout, bool abort_detected, bool stop_detected));
    ~I2CMasterInterface();
    bool readRegister(uint8_t address, uint8_t pointer, uint8_t length,
                      uint8_t *data);
    bool readRegisterRepeatedStart(uint8_t address, uint8_t pointer, uint8_t length,
                                   uint8_t *data);
    bool writeRegister(uint8_t address, uint8_t pointer, uint8_t data);
    bool writeRegister(uint8_t address, uint8_t pointer, uint8_t length,
                       uint8_t *data);
    bool readFrom(uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop);

    bool writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop);
    void dmaWriteRead(
        uint8_t addr,
        const uint8_t *wbuf,
        size_t wbuf_len,
        uint8_t *rbuf,
        size_t rbuf_len);

private:
    i2c_dma_t *i2c_dma = nullptr;
    i2c_inst_t *i2c;
    i2c_inst_t *_hardwareBlocks[NUM_I2CS] = {i2c0, i2c1};
};

class I2CSlaveInterface
{
public:
    void init();
    void on_byte_received(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_received_end(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_requested(uint8_t (*handler)(uint8_t addr));
};