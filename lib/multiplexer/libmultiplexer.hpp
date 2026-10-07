#include <hardware/gpio.h>
#include <hardware/adc.h>
#include <pico/time.h>

#include "stdint.h"
class Multiplexer
{
public:
    Multiplexer(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t input, bool sixteen_channel, bool slow);
    void begin();
    void end();
    uint16_t read(uint8_t channel);

private:
    uint32_t select_mask() const;
    uint8_t s0Pin;
    uint8_t s1Pin;
    uint8_t s2Pin;
    uint8_t s3Pin;
    uint8_t inputPin;
    bool sixteenChannel;
    // CD4051 style parts take a while to settle after switching channels
    bool slow;
};
