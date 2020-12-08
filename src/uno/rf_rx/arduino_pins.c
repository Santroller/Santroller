#include "arduino_pins.h"
// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
const uint16_t PROGMEM port_to_mode_PGM[] = {
    NOT_A_PORT, NOT_A_PORT, (uint16_t)&DDRB, (uint16_t)&DDRC, (uint16_t)&DDRD,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
    NOT_A_PORT,       NOT_A_PORT,       (uint16_t)&PORTB,
    (uint16_t)&PORTC, (uint16_t)&PORTD,
};

const uint16_t PROGMEM port_to_input_PGM[] = {
    NOT_A_PORT, NOT_A_PORT, (uint16_t)&PINB, (uint16_t)&PINC, (uint16_t)&PIND,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
    PD,                             /* 0 */
    PD, PD, PD, PD, PD, PD, PD, PB, /* 8 */
    PB, PB, PB, PB, PB, PC,         /* 14 */
    PC, PC, PC, PC, PC,
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
    _BV(0), /* 0, port D */
    _BV(1), _BV(2), _BV(3), _BV(4), _BV(5), _BV(6),
    _BV(7), _BV(0),                                 /* 8, port B */
    _BV(1), _BV(2), _BV(3), _BV(4), _BV(5), _BV(0), /* 14, port C */
    _BV(1), _BV(2), _BV(3), _BV(4), _BV(5),
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
    NOT_ON_TIMER, /* 0 - port D */
    NOT_ON_TIMER, NOT_ON_TIMER,
// on the ATmega168, digital pin 3 has hardware pwm
#  if defined(__AVR_ATmega8__)
    NOT_ON_TIMER,
#  else
    TIMER2B,
#  endif
    NOT_ON_TIMER,
// on the ATmega168, digital pins 5 and 6 have hardware pwm
#  if defined(__AVR_ATmega8__)
    NOT_ON_TIMER, NOT_ON_TIMER,
#  else
    TIMER0B,      TIMER0A,
#  endif
    NOT_ON_TIMER, NOT_ON_TIMER, /* 8 - port B */
    TIMER1A,      TIMER1B,
#  if defined(__AVR_ATmega8__)
    TIMER2,
#  else
    TIMER2A,
#  endif
    NOT_ON_TIMER, NOT_ON_TIMER, NOT_ON_TIMER, NOT_ON_TIMER, /* 14 - port C */
    NOT_ON_TIMER, NOT_ON_TIMER, NOT_ON_TIMER, NOT_ON_TIMER,
};