#pragma once

#include "uart.hpp"

#define PY_CHIP_PID 0x0064
#define PY_BLOCKSIZE 128
#define PY_SECTORSIZE 4096
#define PY_FLASH_ADDR 0x08000000
#define PY_CODE_ADDR 0x08001000
#define PY_SRAM_ADDR 0x20000000
#define PY_BOOT_ADDR 0x1fff0000
#define PY_UID_ADDR 0x1fff0e00
#define PY_OPTION_ADDR 0x1fff0080
#define PY_CONFIG_ADDR 0x1fff0f00

// # Command codes
#define PY_CMD_GET 0x00
#define PY_CMD_VER 0x01
#define PY_CMD_PID 0x02
#define PY_CMD_READ 0x11
#define PY_CMD_WRITE 0x31
#define PY_CMD_ERASE 0x44
#define PY_CMD_GO 0x21
#define PY_CMD_W_LOCK 0x63
#define PY_CMD_W_UNLOCK 0x73
#define PY_CMD_R_LOCK 0x82
#define PY_CMD_R_UNLOCK 0x92

// # Reply codes
#define PY_REPLY_ACK 0x79
#define PY_REPLY_NACK 0x1f
#define PY_REPLY_BUSY 0xaa

// # Other codes
#define PY_SYNCH 0x7f

// # Default option bytes
#define PY_OPTION_DEFAULT = b '\xaa\xbe\x55\x41\xff\x00\x00\xff\xff\xff\xff\xff\xff\xff\x00\x00'

typedef struct
{
    // uint8_t header0 = 0xa5; byte chomped as its start byte
    uint8_t header = 0x01;
    uint8_t len = 0x0C;
    uint8_t padding[2] = {0x00, 0x00};
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;
    uint8_t dpadUpDown;    // none: 0x80, up: 0x00, down: 0xFF
    uint8_t dpadLeftRight; // none: 0x80, right: 0x00, left: 0xFF
    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t : 3;
    uint8_t footer[2] = {0x01, 0x15}; // gh: {0x01, 0x15}, rb: {0x03, 0x20}
    uint8_t crc;                      // CRC-8/MAXIM-DOW
} __attribute__((packed)) crkd_neck_t;
class CrkdNeck
{
public:
    CrkdNeck(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock)
        : interface(block, tx, rx, clock) {};
    void tick();
    inline bool isConnected()
    {
        return m_connected;
    }
    bool green;
    bool red;
    bool yellow;
    bool blue;
    bool orange;
    bool soloGreen;
    bool soloRed;
    bool soloYellow;
    bool soloBlue;
    bool soloOrange;
    bool dpadUp;
    bool dpadDown;
    bool dpadLeft;
    bool dpadRight;
    bool rb;

private:
    void update_firmware();
    void writeflash(uint32_t addr, uint8_t *data, uint32_t size);
    void readflash(uint32_t addr, uint16_t size, uint8_t *out);
    void erase(uint16_t size);
    void read_info();
    void read_info_stream(uint8_t cmd, uint8_t *out);
    void send_command(uint8_t cmd);
    bool check_reply();
    void sendaddress(uint32_t addr);
    UARTInterface interface;
    bool m_connected = false;
    bool m_updating = false;
    crkd_neck_t m_crkdNeck;
    long m_lastPoll;
    bool m_error = false;
    long m_lastSend;
};
