#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdint.h>

#include "Arduino.h"
#include "config.h"
#include "pins_define.h"
#include "util.h"

uint16_t adcReading[ADC_COUNT];
const uint8_t analogPins[ADC_COUNT] = ADC_PINS;
bool first = true;
uint16_t adc(uint8_t pin) {
    return adcReading[pin];
}

const uint16_t PROGMEM ports[PORT_COUNT] = PORTS;
int currentAnalog = 0;
void tickAnalog(void) {
#if ADC_COUNT != 0
    if (!first) {
        if (bit_is_set(ADCSRA, ADSC)) return;
        uint8_t low, high;
        low = ADCL;
        high = ADCH;
        uint16_t data = (high << 8) | low;
        adcReading[currentAnalog] = data << 6;
        currentAnalog++;
        if (currentAnalog == ADC_COUNT) {
            currentAnalog = 0;
        }
    }
    first = false;
    uint8_t pin = analogPins[currentAnalog];

#if defined(ADCSRB) && defined(MUX5)
    // the MUX5 bit of ADCSRB selects whether we're reading from channels
    // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
    ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

    // set the analog reference (high two bits of ADMUX) and select the
    // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
    // to 0 (the default).

    ADMUX = (1 << 6) | (pin & 0x07);

    sbi(ADCSRA, ADSC);
#endif
}

uint8_t digital_read(uint8_t port_num, uint8_t mask) {
    volatile uint8_t* port = ((volatile uint8_t*)(pgm_read_word(ports + port_num)));
    // ddr is one memory address above port
    volatile uint8_t* ddr = port - 1;
    volatile uint8_t* pin = port - 2;
    uint8_t prevPort = *port;
    uint8_t prevDdr = *ddr;
    uint8_t oldSREG = SREG;
    cli();
    *port |= mask;
    // And write it inverted to ddr (ones set pullup)
    *ddr &= ~mask;
    uint8_t data = *pin;
    // Revert the settings we changed
    *port = prevPort;
    *ddr &= prevDdr;
    SREG = oldSREG;
    return data;
}

uint16_t adc_read(uint8_t pin, uint8_t mask) {
    bool reset = false;
    // When we are not doing pin detection, we don't want to be fiddling with the ports
    if (pin & (1 << 7)) {
        pin = pin & ~(1 << 7);
        reset = true;
    }

    volatile uint8_t* port = ANALOG_PORT(pin);
    // ddr is one memory address above port
    volatile uint8_t* ddr = port - 1;
    uint8_t prevPort = *port;
    uint8_t prevDdr = *ddr;
    if (reset) {
        uint8_t oldSREG = SREG;
        cli();
        *port |= mask;
        // And write it inverted to ddr (ones set pullup)
        *ddr &= ~mask;
        SREG = oldSREG;
    }
    while (bit_is_set(ADCSRA, ADSC))
        ;
    first = true;
#if defined(ADCSRB) && defined(MUX5)
    // the MUX5 bit of ADCSRB selects whether we're reading from channels
    // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
    ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

    // set the analog reference (high two bits of ADMUX) and select the
    // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
    // to 0 (the default).

    ADMUX = (1 << 6) | (pin & 0x07);

    sbi(ADCSRA, ADSC);
    while (bit_is_set(ADCSRA, ADSC))
        ;
    uint8_t low, high;
    low = ADCL;
    high = ADCH;
    uint16_t data = (high << 8) | low;
    data = data << 6;
    if (reset) {
        // Revert the settings we changed
        uint8_t oldSREG = SREG;
        cli();
        *port = prevPort;
        *ddr &= prevDdr;
        SREG = oldSREG;
    }
    return data;
}
void initPins(void) {
    PIN_INIT;
    tickAnalog();
}

void tickPins(void) {
    tickAnalog();
}