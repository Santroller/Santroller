#include "multiplexer.hpp"
Multiplexer::Multiplexer(uint8_t s0Pin, uint8_t s1Pin, uint8_t s2Pin, uint8_t inputPin): s0Pin(s0Pin), s1Pin(s1Pin), s2Pin(s2Pin), s3Pin(0), sixteenChannel(false) {
    gpio_init_mask(1 << s0Pin | 1 << s1Pin | 1 << s2Pin);
    gpio_set_dir_out_masked(1 << s0Pin | 1 << s1Pin | 1 << s2Pin);
}
Multiplexer::Multiplexer(uint8_t s0Pin, uint8_t s1Pin, uint8_t s2Pin, uint8_t s3Pin, uint8_t inputPin): s0Pin(s0Pin), s1Pin(s1Pin), s2Pin(s2Pin), s3Pin(s3Pin), sixteenChannel(true) {
    gpio_init_mask(1 << s0Pin | 1 << s1Pin | 1 << s2Pin | 1 << s3Pin);
    gpio_set_dir_out_masked(1 << s0Pin | 1 << s1Pin | 1 << s2Pin | 1 << s3Pin);
}
uint16_t Multiplexer::read(uint8_t channel) {
    gpio_put(s0Pin, channel & 0b0001);
    gpio_put(s1Pin, channel & 0b0010);
    gpio_put(s2Pin, channel & 0b0100);
    if (sixteenChannel) {
        gpio_put(s3Pin, channel & 0b1000);
    }
    adc_select_input(inputPin);
    return adc_read() << 4;
}