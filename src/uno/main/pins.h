#include <avr/pgmspace.h>
#include <stdint.h>
#define PA 1
#define PB 2
#define PC 3
#define PD 4
#define PE 5
#define PF 6
#define PG 7
#define PH 8
#define PJ 10
#define PK 11
#define PL 12
#define NOT_A_PORT 0
#define NOT_A_PIN 0
// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
extern const uint16_t PROGMEM port_to_mode_PGM[] = {
	NOT_A_PORT,
	NOT_A_PORT,
	(uint16_t) &DDRB,
	(uint16_t) &DDRC,
	(uint16_t) &DDRD,
};
extern const uint16_t PROGMEM port_to_input_PGM[] = {
    NOT_A_PORT, NOT_A_PORT, (uint16_t)&PINB, (uint16_t)&PINC, (uint16_t)&PIND,
};

extern const uint16_t PROGMEM port_to_output_PGM[] = {
    NOT_A_PORT,       NOT_A_PORT,       (uint16_t)&PORTB,
    (uint16_t)&PORTC, (uint16_t)&PORTD,
};

extern const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
    PD,                             /* 0 */
    PD, PD, PD, PD, PD, PD, PD, PB, /* 8 */
    PB, PB, PB, PB, PB, PC,         /* 14 */
    PC, PC, PC, PC, PC,
};

extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
    _BV(0), /* 0, port D */
    _BV(1), _BV(2), _BV(3), _BV(4), _BV(5), _BV(6),
    _BV(7), _BV(0),                                 /* 8, port B */
    _BV(1), _BV(2), _BV(3), _BV(4), _BV(5), _BV(0), /* 14, port C */
    _BV(1), _BV(2), _BV(3), _BV(4), _BV(5),
};