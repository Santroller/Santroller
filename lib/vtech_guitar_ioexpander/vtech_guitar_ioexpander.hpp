#pragma once
#include <stdint.h>

#include "spi.hpp"
#define CS_DELAY 2000

// VTech Guitar SPI IO Expander Register Definitions (Write = 0x80 | Reg)
#define VTECH_REG_HANDSHAKE_READ   0x00 // Returns 0x5A when initialized
#define VTECH_REG_P0_OUT           0x81 // Port 0 Output Data
#define VTECH_REG_P1_OUT           0x82 // Port 1 Output Data
#define VTECH_REG_P0_DIR           0x84 // Port 0 Direction (0xFF = Input, 0x00 = Output)
#define VTECH_REG_P1_DIR           0x85 // Port 1 Direction
#define VTECH_REG_P2_DIR           0x86 // Port 2 Direction
#define VTECH_REG_KEY1             0x88 // Unlock Key 1 (0xA5)
#define VTECH_REG_KEY2             0x80 // Unlock Key 2 (0x5A)
#define VTECH_REG_P0_PULLUP        0x89 // Port 0 Pull-Up Enable
#define VTECH_REG_P1_PULLUP        0x8A // Port 1 Pull-Up Enable
#define VTECH_REG_POLL_INPUTS      0x0E // Read Button Inputs
#define VTECH_REG_SOFT_RESET       0xFF // Soft Reset Command

typedef enum 
{
    INIT_SOFT_RESET,       // Send 0xFF 0x00 (Soft Reset)
    INIT_UNLOCK_KEY1,     // Send 0x88 0xA5 (Unlock Key 1)
    INIT_UNLOCK_KEY2,     // Send 0x80 0x5A (Unlock Key 2)
    INIT_SET_P0_DIR_IN,    // Send 0x84 0xFF (Set Port 0 Direction -> Input)
    INIT_SET_P0_PULLUP,    // Send 0x89 0xFF (Enable Port 0 Pull-ups)
    INIT_SET_P1_DIR_IN,    // Send 0x85 0xFF (Set Port 1 Direction -> Input)
    INIT_SET_P0_OUT_CLEAR, // Send 0x81 0x00 (Clear Port 0 Output Data)
    INIT_SET_P1_PULLUP,    // Send 0x8A 0xFF (Enable Port 1 Pull-ups)
    INIT_SET_P2_DIR_OUT,   // Send 0x86 0x00 (Set Port 2 Direction -> Output for LEDs)
    INIT_SET_P1_OUT_OFF,   // Send 0x82 0xFF (Turn off active-low LEDs)
    CHECK_HANDSHAKE,       // Read 0x00 (Verify handshake response 0x5A)
    POLL_INPUTS,           // Read 0x0E (Poll button inputs)
    UPDATE_LEDS            // Write 0x81 (Write LED output state)
} VTechGuitarIOExpanderState;
class VTechGuitarIOExpander {
   public:
    VTechGuitarIOExpander(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t csPin);
    void tick();
    void begin();
    void end();
    void process_data(bool ack, bool timeout);
    void no_attention();
    void signal_attention();
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