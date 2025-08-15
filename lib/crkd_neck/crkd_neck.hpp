#pragma once

#include "uart.hpp"

typedef struct
{
    uint8_t header; // {0xA5, 0x01};, though we are eating the first header byte
    uint8_t len; // 0x0C;
    uint8_t padding[2]; //{0x00, 0x00};
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;
    uint8_t dpadUpDown;    // none: 0x80, up: 0x00, down: 0xFF
    uint8_t dpadLeftRight; // none: 0x80, right: 0x00, left: 0xFF
    uint8_t footer[3]; // {0x00, 0x01, 0x15};
    uint8_t crc; // CRC-8/MAXIM-DOW
} __attribute__((packed)) crkd_neck_t;
class CrkdNeck {
   public:
    CrkdNeck(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock)
        : interface(block, tx, rx, clock) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }
    bool green;
    bool red;
    bool yellow;
    bool blue;
    bool orange;
    bool dpadUp;
    bool dpadDown;
    bool dpadLeft;
    bool dpadRight;

   private:
    UARTInterface interface;
    bool connected;
    crkd_neck_t m_crkdNeck;
};
