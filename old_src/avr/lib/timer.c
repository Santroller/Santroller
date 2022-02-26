#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>

#include "avr.h"
#define clockCyclesPerMicrosecond() (realFreq / 1000000L)

#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())

#define microsecondsToClockCycles(a) ((a)*clockCyclesPerMicrosecond())

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

volatile unsigned long timer0_overflow_count = 0;

uint32_t realFreq = F_CPU;
bool scaleDiv2 = false;
bool scaleMul2 = false;

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
    defined(__AVR_ATtiny84__)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{
    timer0_overflow_count++;
}

// there are 1024 microseconds per overflow counter tick.
unsigned long millis(void) {
    unsigned long m;
    uint8_t oldSREG = SREG;

    // disable interrupts while we read FastLED_timer0_millis or we might get an
    // inconsistent value (e.g. in the middle of a write to FastLED_timer0_millis)
    cli();
    m = timer0_overflow_count;  //._long;
    SREG = oldSREG;

    return (m * (MICROSECONDS_PER_TIMER0_OVERFLOW / 8)) / (1000 / 8);
}

unsigned long micros(void) {
    unsigned long m;
    uint8_t oldSREG = SREG, t;

    cli();
    m = timer0_overflow_count;  // ._long;
#if defined(TCNT0)
    t = TCNT0;
#elif defined(TCNT0L)
    t = TCNT0L;
#else
#error TIMER 0 not defined
#endif

#ifdef TIFR0
    if ((TIFR0 & _BV(TOV0)) && (t < 255)) m++;
#else
    if ((TIFR & _BV(TOV0)) && (t < 255)) m++;
#endif

    SREG = oldSREG;

    return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}

void setupMicrosTimer(void) {
    // set timer 0 prescale factor to 64
#if defined(__AVR_ATmega128__)
    // CPU specific: different values for the ATmega128
    sbi(TCCR0, CS02);
#elif defined(TCCR0) && defined(CS01) && defined(CS00)
    // this combination is for the standard atmega8
    sbi(TCCR0, CS01);
    sbi(TCCR0, CS00);
#elif defined(TCCR0B) && defined(CS01) && defined(CS00)
    // this combination is for the standard 168/328/1280/2560
    sbi(TCCR0B, CS01);
    sbi(TCCR0B, CS00);
#elif defined(TCCR0A) && defined(CS01) && defined(CS00)
    // this combination is for the __AVR_ATmega645__ series
    sbi(TCCR0A, CS01);
    sbi(TCCR0A, CS00);
#else
#error Timer 0 prescale factor 64 not set correctly
#endif

    // enable timer 0 overflow interrupt
#if defined(TIMSK) && defined(TOIE0)
    sbi(TIMSK, TOIE0);
#elif defined(TIMSK0) && defined(TOIE0)
    sbi(TIMSK0, TOIE0);
#else
#error Timer 0 overflow interrupt not set correctly
#endif
}

#ifdef __AVR_ATmega32U4__
void delay_ms(uint32_t __ms) {
    if (scaleDiv2) {
        _delay_ms(__ms >> 1);
    } else if (scaleMul2) {
        _delay_ms(__ms << 1);
    } else {
        _delay_ms(__ms);
    }
}
void delay_us(uint32_t __us) {
    if (scaleDiv2) {
        _delay_us(__us >> 1);
    } else if (scaleMul2) {
        _delay_us(__us << 1);
    } else {
        _delay_us(__us);
    }
}
#else
void delay_ms(uint32_t __ms) {
    _delay_ms(__ms);
}
void delay_us(uint32_t __us) {
    _delay_us(__us);
}
#endif