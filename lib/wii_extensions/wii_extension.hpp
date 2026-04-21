#pragma once
#include <stdint.h>
#include "i2c.hpp"
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "devices/midi.hpp"
#define WII_ADDR 0x52
#define WII_READ_ID 0xFA
#define WII_ENCRYPTION_STATE_ID 0xF0
#define WII_ENCRYPTION_ENABLE_ID 0xAA
#define WII_ENCRYPTION_FINISH_ID 0x55
#define WII_ENCRYPTION_KEY_ID 0x40
#define WII_ENCRYPTION_KEY_ID_2 0x46
#define WII_ENCRYPTION_KEY_ID_3 0x4C
#define WII_ID_LEN 6
#define WII_DJ_EUPHORIA 0xFB
#define WII_SET_RES_MODE 0xFE
#define WII_LOWRES_MODE 0x03
#define WII_HIGHRES_MODE 0x03
#define FIRST_PARTY_SBOX 0x97
#define THIRD_PARTY_SBOX 0x4D

#define I2C_MAX_TRANSFER_SIZE 1056
// A transfer timeout of 1000ms will allow a 10000 bit transfer to complete
// successfully without timeouts at baudrates as low as 10000 baud.
#define I2C_TRANSFER_TIMEOUT_MS 10000
#define I2C_TAKE_MUTEX_TIMEOUT_MS 10000
typedef enum {
    I2C_NONE,
    I2C_WRITE,
    I2C_READ_POINTER,
    I2C_READ
} i2c_status_e;
typedef enum {
    WII_INIT_FINISH_ENC,
    WII_INIT_FB_0,
    WII_INIT_READ_ID_WRITE_PTR,
    WII_INIT_READ_ID_READ,
    WII_INIT_DRAWSOME,
    WII_INIT_CLASSIC_0,
    WII_INIT_CLASSIC_1,
    WII_INIT_CLASSIC_2,
    WII_INIT_CLASSIC_READ_ID_WRITE_PTR,
    WII_INIT_CLASSIC_READ_ID_READ,
    WII_INIT_READ_DATA_WRITE_PTR,
    WII_INIT_READ_DATA_READ,
    WII_INIT_ENABLE_ENC_0,
    WII_INIT_ENABLE_ENC_1,
    WII_INIT_ENABLE_ENC_2,
    WII_INIT_ENABLE_ENC_3,
    WII_INIT_ENC_READ_ID_WRITE_PTR,
    WII_INIT_ENC_READ_ID_READ,
    WII_INPUTS_WRITE_PTR,
    WII_INPUTS_READ,
} wii_status_e;
typedef struct i2c_dma_s
{
    i2c_status_e status;
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
    alarm_id_t restart_alarm_id;

    uint16_t data_cmds[I2C_MAX_TRANSFER_SIZE];
    void (*process_data)();
} i2c_dma_t;
class WiiExtension
{

public:
    WiiExtension(MidiDevice* midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock);
    ~WiiExtension();
    void tick();
    void processData();
    WiiExtType mType = WiiExtType::WiiNoExtension;
    uint16_t readAxis(proto_WiiAxisType type);
    bool readButton(proto_WiiButtonType type);
    uint8_t mBuffer[8];

private:
    bool verifyData(const uint8_t *dataIn, uint8_t dataSize);
    void setEuphoriaLed(bool state);
    I2CMasterInterface mInterface;
    bool mFound;
    bool nextEuphoriaLedState = false;
    bool ledUpdated = false;
    bool hadDrum = false;
    uint8_t packetIssueCount;
    uint8_t mBufferIndex;
    long lastTick;
    uint8_t wiiBytes;
    uint8_t wiiPointer = 0;
    uint8_t s_box = 0;
    MidiDevice *m_device;

    bool started = false;
    bool delayNext = false;

    bool hiRes = false;
    bool hasTapBar = false;

    i2c_inst_t *i2c;
    i2c_dma_t *i2c_dma;
    i2c_inst_t* _hardwareBlocks[NUM_I2CS] = {i2c0,i2c1};
    wii_status_e status = WII_INIT_FINISH_ENC;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
};