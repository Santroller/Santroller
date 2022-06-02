#pragma once
#include "config/config.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#ifndef _BV
#  define _BV(bit) (1 << (bit))
#endif
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
// #define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_write(c, p, m) ((p) = (p & ~(_BV(m))) | ((!!(c)) << m))
#define bit_check(value, bit) (value & _BV(bit))

#define constrain(amt, low, high)                                              \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define min(a, b) a < b ? a : b
#define STR1(x) #x
#define STR(x) STR1(x)
#define PASTE(x, y) x##y
#define MAKEWIDE(x) PASTE(L, x)

#define NOT_A_PIN 0
#define NOT_A_PORT 0
#define OUTPUT 0
#define INPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLUP_ANALOG 3
#define NOT_ON_TIMER 0
#define TIMER0A 1
#define TIMER0B 2
#define TIMER1A 3
#define TIMER1B 4
#define TIMER1C 5
#define TIMER2 6
#define TIMER2A 7
#define TIMER2B 8

#define TIMER3A 9
#define TIMER3B 10
#define TIMER3C 11
#define TIMER4A 12
#define TIMER4B 13
#define TIMER4C 14
#define TIMER4D 15
#define TIMER5A 16
#define TIMER5B 17
#define TIMER5C 18
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
#ifndef cbi
#  define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#  define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
extern uint8_t reverse(uint8_t b);
#define NOT_AN_INTERRUPT -1
#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)

#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())

#define microsecondsToClockCycles(a) ((a)*clockCyclesPerMicrosecond())

#if __AVR__
#  include <avr/interrupt.h>
#else
#  include "hardware/sync.h"
extern void sei();
extern void cli();
inline uint8_t pgm_read_byte(const uint8_t *ptr) { return *ptr; }
#endif