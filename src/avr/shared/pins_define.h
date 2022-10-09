#ifdef __AVR_ATmega328P__
#define NUM_TOTAL_PINS 24
#define PORT_COUNT 3
#define PORTS \
    { (uint16_t) &PORTB, (uint16_t)&PORTC, (uint16_t)&PORTD }
// PC0-5
#define ANALOG_MASK \
    { 0, 0b11111, 0 }
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define NUM_TOTAL_PINS 70
#define PORT_COUNT 11
#define PORTS \
    { (uint16_t) &PORTA, (uint16_t)&PORTB, (uint16_t)&PORTC, (uint16_t)&PORTD, (uint16_t)&PORTE, (uint16_t)&PORTF, (uint16_t)&PORTG, (uint16_t)&PORTH, (uint16_t)&PORTJ, (uint16_t)&PORTK, (uint16_t)&PORTL }
// PF and PK
#define ANALOG_MASK \
    { 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0 }
#elif defined(__AVR_ATmega32U4__)
#define NUM_TOTAL_PINS 24
#define PORT_COUNT 5
#define PORTS \
    { (uint16_t) &PORTB, (uint16_t)&PORTC, (uint16_t)&PORTD, (uint16_t)&PORTE, (uint16_t)&PORTF }
// PF0-1 PF4-7 PB4-6 PD4,6,7
#define ANALOG_MASK \
    { 0b0001111, 0, 0b00001011, 0, 0b1101111 }
#elif defined(__AVR_AT90USB1286__)
#define NUM_TOTAL_PINS 44
#define PORT_COUNT 6
#define PORTS \
    { (uint16_t) &PORTA, (uint16_t) &PORTB, (uint16_t)&PORTC, (uint16_t)&PORTD, (uint16_t)&PORTE, (uint16_t)&PORTF }
// PF
#define ANALOG_MASK \
    { 0, 0, 0, 0, 0, 0xff }
#endif


#define TWI_BLOCK void*
#define SPI_BLOCK void*
#define GC_TWI NULL
#define GC_SPI NULL