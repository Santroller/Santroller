#include "pin.h"

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "avr.h"
#include "config.h"
#include "lib_main.h"
#include "string.h"
static Bindings_t* bindingsPointer = (Bindings_t*)0;
uint8_t analogCount = 0;
int currentAnalog = 0;
void setupADC(void);
void initPins(void) {
#ifdef PINA
#define FIRST_PIN &PINA
#else
#define FIRST_PIN &PINB
#endif
    // By resetting analogCount and clearing the adc interrupt, we can run this function again when the config is modified
    analogCount = 0;
    pinCount = 0;
    cbi(ADCSRA, ADIE);
    Binding_t* binding = (Binding_t*)buf;
    AnalogData_t* analog = (AnalogData_t*)buf + sizeof(Binding_t);
    bool isMouse = true;
    // If an input has mouse bindings, we want them to appear first.
    // To do this, we loop twice, once skipping mouse bindings and once skipping keyboard bindings.
    while (true) {
        for (int i = 0; i < PORTS * PINS_PER_PORT; i++) {
            eeprom_read_block(binding, &bindingsPointer->bindings[i], sizeof(Binding_t));
            if (isMouse != (binding->type == DIRECT_MOUSE || binding->type == OTHER_MOUSE)) continue;
            if (binding->type) {
                volatile uint8_t* pinx = FIRST_PIN + ((i / PINS_PER_PORT) * 3);
// For the mega, there are two sections to get pins from, it splits to another memory section after PING
#ifdef PINH
                if ((i / PINS_PER_PORT) > 7) {
                    pinx = (&PINH) + ((i / PINS_PER_PORT) * 3);
                }
#endif
                volatile uint8_t* ddrx = pinx + 1;
                volatile uint8_t* portx = pinx + 2;
                uint8_t bit = _BV(i % PINS_PER_PORT);
                pins[i].port = pinx;
                pins[i].mask = bit;
                pins[i].binding = binding->binding;
                pins[i].pullup = binding->pullup;
                pins[i].isExt = binding->type == DIRECT_TYPE || binding->type == DIRECT_MOUSE;
                if (pins[i].isExt) {
                    pins[i].digitalRead = readDigital;
                    pins[i].axisInfo = NULL;
                    if (binding->analogID) {
                        eeprom_read_block(analog, &bindingsPointer->analog[binding->analogID], sizeof(AnalogData_t));
                        pins[i].axisInfo = &analogInfo[binding->analogID];
                        pins[i].axisInfo->deadzone = analog->scale.deadzone;
                        pins[i].axisInfo->offset = analog->scale.offset;
                        pins[i].axisInfo->multiplier = analog->scale.multiplier;
                        pins[i].axisInfo->isADC = analog->mapToDigital;
                        pins[i].axisInfo->ADCtrigger = analog->trigger;
                        pins[i].axisInfo->channel = analog->channel;
                        pins[i].axisInfo->analogRead = readAnalog;
                    } else {
                        uint8_t oldSREG = SREG;
                        cli();
                        *ddrx &= ~bit;
                        if (binding->pullup) {
                            *portx |= bit;
                        } else {
                            *portx &= ~bit;
                        }
                        SREG = oldSREG;
                    }
                }
                pinCount++;
            }
        }
        if (!isMouse) {
            break;
        }
        firstKeyboardPin = pinCount;
        isMouse = false;
    }
    setupADC();

    if (analogCount > 0) {
        uint8_t pin = analogInfo[currentAnalog].currentValue;
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
}
void setDefaults(void) {
    // 0xFF is used everywhere as a flag for something being disabled.
    for (size_t i = 0; i < sizeof(Bindings_t); i++) {
        eeprom_update_byte(((uint8_t*)&bindingsPointer) + i, 0x00);
    }
}

bool readDigital(Input_t* pin) {
    if (pin->axisInfo) {
        return readAnalog(pin) > pin->axisInfo->ADCtrigger;
    }
    return ((*pin->port & pin->mask) != 0) != pin->pullup;
}

int16_t readAnalog(Input_t* pin) {
    if (!pin->axisInfo) {
        return readDigital(pin) * INT16_MAX;
    }
    return pin->axisInfo->currentValue;
}

// Read data from the ADC into analogInfo when adc is ready, and then round-robin the next input
ISR(ADC_vect) {
    analogInfo[currentAnalog].currentValue = (ADC - 512) << 5;
    currentAnalog++;
    if (currentAnalog == analogCount) {
        currentAnalog = 0;
    }
    uint8_t pin = analogInfo[currentAnalog].channel;
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

// Enable ADC
void setupADC() {
#if defined(ADCSRA)
// set a2d prescaler so we are inside the desired 50-200 KHz range.
#if F_CPU >= 16000000  // 16 MHz / 128 = 125 KHz
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
#elif F_CPU >= 8000000  // 8 MHz / 64 = 125 KHz
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
#elif F_CPU >= 4000000  // 4 MHz / 32 = 125 KHz
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
#elif F_CPU >= 2000000  // 2 MHz / 16 = 125 KHz
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
#elif F_CPU >= 1000000  // 1 MHz / 8 = 125 KHz
    cbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
#else                   // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the prescaler is 2
    cbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
#endif
    // enable a2d conversions
    sbi(ADCSRA, ADEN);
#endif
    // Enable adc interrupts
    sbi(ADCSRA, ADIE);
}