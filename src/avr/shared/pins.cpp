#include <avr/io.h>
#include <inttypes.h>
#include <stdint.h>

#include "Arduino.h"
#include "config.h"
#include "io_define.h"
#include "progmem.h"
#include "util.h"

volatile uint16_t adcReading[ADC_COUNT];
const uint8_t analogPins[ADC_COUNT] = ADC_PINS;
const uint16_t PROGMEM ports[PORT_COUNT] = PORTS;
uint16_t adc(uint8_t pin) {
    return adcReading[pin];
}

uint8_t digital_read(uint8_t port_num, uint8_t mask) {
    volatile uint8_t* port = ((volatile uint8_t*)(pgm_read_word(ports + port_num)));
    volatile uint8_t* ddr = port - 1;
    volatile uint8_t* pin = port - 2;
    uint8_t prevPort = *port;
    uint8_t prevDdr = *ddr;
    uint8_t oldSREG = SREG;
    cli();
    // Set direction to input
    *ddr &= ~mask;
    // Enable pullup
    *port |= mask;
    delay(10);
    uint8_t data = *pin;
    // Revert the settings we changed
    *port = prevPort;
    *ddr = prevDdr;
    SREG = oldSREG;
    return data;
}

uint16_t adc_read(uint8_t pin, uint8_t mask) {
#if ADC_COUNT != 0
    cbi(ADCSRA, ADIE);
    while (bit_is_set(ADCSRA, ADSC))
        ;
#endif
    // bit 7 is used as a flag for pin detection mode
    bool detection = pin & (1 << 7);
    pin &= ~(1 << 7);

    // When we are not doing pin detection, we don't want to be fiddling with the ports
    volatile uint8_t* port = ANALOG_PORT(pin);
    volatile uint8_t* ddr = port - 1;
    uint8_t prevPort = *port;
    uint8_t prevDdr = *ddr;
    if (detection) {
        uint8_t oldSREG = SREG;
        cli();
        // Clear DDRx (input)
        *ddr &= ~mask;
        // Set PORTx (pull_up)
        *port |= mask;
        SREG = oldSREG;
    }
#if defined(ADCSRB) && defined(MUX5)
    // the MUX5 bit of ADCSRB selects whether we're reading from channels
    // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
    ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

    // set the analog reference (high two bits of ADMUX) and select the
    // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
    // to 0 (the default).

    ADMUX = (1 << 6) | (pin & 0x07);

    // Give things time to settle
    delayMicroseconds(10);
    sbi(ADCSRA, ADSC);
    while (bit_is_set(ADCSRA, ADSC))
        ;
    uint16_t data = ADC << 6;
    if (detection) {
        // Revert the settings we changed
        uint8_t oldSREG = SREG;
        cli();
        *port = prevPort;
        *ddr = prevDdr;
        SREG = oldSREG;
    }
#if ADC_COUNT != 0
    sbi(ADCSRA, ADIE);
    sbi(ADCSRA, ADSC);
#endif
    return data;
}
void initPins(void) {
    PIN_INIT;
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
#if ADC_COUNT != 0
    uint8_t pin = analogPins[0];

#if defined(ADCSRB) && defined(MUX5)
    // the MUX5 bit of ADCSRB selects whether we're reading from channels
    // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
    ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

    // set the analog reference (high two bits of ADMUX) and select the
    // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
    // to 0 (the default).

    ADMUX = (1 << 6) | (pin & 0x07);

    sbi(ADCSRA, ADIE);
    sbi(ADCSRA, ADSC);
#endif
}
int currentAnalog = 0;
#if ADC_COUNT != 0
ISR(ADC_vect) {
    adcReading[currentAnalog] = ADC << 6;
    currentAnalog++;
    if (currentAnalog == ADC_COUNT) {
        currentAnalog = 0;
    }
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
}
#endif