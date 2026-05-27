#pragma once
#include <stdint.h>

#include "spi.hpp"
#define CS_DELAY 2000

typedef enum 
{
    INIT_POWER_ON,
    INIT_2,
    INIT_3,
    INIT_4,
    INIT_5,
    INIT_6,
    INIT_7,
    INIT_8,
    INIT_9,
    INIT_10,
    CHECK,
    POLL,
    UPDATE_LED
} VTechGuitarIOExpanderState;
class VTechGuitarIOExpander {
   public:
    VTechGuitarIOExpander(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t csPin)
        : mInterface(block, SPI_CPHA_1, SPI_CPOL_1, sck, mosi, miso, false, clock), mCsPin(csPin) {};
    void tick();
    void begin();
    void end();
    void processData(bool ack, bool timeout);
    void noAttention();
    void signalAttention();
    bool read_button(uint8_t pin);
    void set_led(uint8_t i, uint8_t val);
    inline bool is_connected() {
        return connected;
    }

   private:
    SPIMasterInterface mInterface;
    uint8_t mCsPin;
    int missing;
    bool connected;
    bool attention = false;
    uint8_t button_data = 0;
    uint8_t led_data = 0;
    VTechGuitarIOExpanderState status;
    alarm_id_t timeout_alarm_id;
};