#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <stdint.h>

#include "Arduino.h"
#include "config.h"
#include "io_define.h"
#include "pin_funcs.h"
#include "util.h"
#include "ws2812.pio.h"
uint16_t adcReading[NUM_ANALOG_INPUTS];
bool first = true;
uint16_t adc(uint8_t pin) {
    adc_select_input(pin);
    return adc_read() << 4;
}
PIO ws2812Pio;
uint ws2812Sm;
uint ws2812Offset;
void putWs2812(uint32_t pixel) {
     pio_sm_put_blocking(ws2812Pio, ws2812Sm, pixel);
}
void initPins(void) {
#ifdef WS2812_PIN
    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &ws2812Pio, &ws2812Sm, &ws2812Offset, WS2812_PIN, 1, true);
    ws2812_program_init(ws2812Pio, ws2812Sm, ws2812Offset, WS2812_PIN, 800000, LED_COUNT_WS2812W);
#endif
    adc_init();
    PIN_INIT;
}

uint8_t digital_read(uint8_t port, uint8_t mask) {
    port = port * 8;
    uint32_t mask32 = mask << port;
    for (uint i = 0; i < NUM_BANK0_GPIOS; i++) {
        if (mask32 & (1 << i)) {
            gpio_init(i);
            gpio_set_pulls(i, true, false);
        }
    }
    uint32_t ret = sio_hw->gpio_in;
    PIN_INIT;
    return ((ret >> port) & 0xff);
}

void digital_write(uint8_t port, uint8_t mask, uint8_t activeMask) {
    port = port * 8;
    uint32_t mask32 = mask << port;
    uint32_t activeMask32 = activeMask << port;
    for (uint i = 0; i < NUM_BANK0_GPIOS; i++) {
        if (mask32 & (1 << i)) {
            gpio_init(i);
            gpio_set_dir(i, true);
        }
    }
// If someone attempts to write to pin 25, assume they want to control the led and proxy
// the call to the cyw43
#if BLUETOOTH
    if (mask32 & 1 << 25) {
        cyw43_arch_gpio_put(0, activeMask32 & 1 << 25);
    }
#endif
    gpio_put_masked(mask32, activeMask32);
}

uint16_t adc_read(uint8_t pin, uint8_t mask) {
    bool detecting = pin & (1 << 7);
    if (detecting) {
        pin = pin & ~(1 << 7);
        gpio_init(pin + PIN_A0);
        gpio_set_pulls(pin + PIN_A0, true, false);
        gpio_set_input_enabled(pin + PIN_A0, false);
    }
    adc_select_input(pin);
    uint16_t data = adc_read() << 4;
    if (detecting) {
        PIN_INIT;
    }
    return data;
}

uint16_t multiplexer_read(uint8_t pin, uint32_t mask, uint32_t bits) {
    if (!disable_multiplexer) {
        gpio_put_masked(mask, bits);
#ifdef CD4051BE
        sleep_us(50);
#endif
        adc_select_input(pin);
        return adc_read() << 4;
    }
    return 0;
}