#pragma once
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sfr_defs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "config/config.h"
#include "output/descriptors.h"
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
#define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_check(value, bit) (value & _BV(bit))

#define constrain(amt, low, high)                                              \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define STR1(x) #x
#define STR(x) STR1(x)
#define PASTE(x, y) x##y
#define MAKEWIDE(x) PASTE(L, x)

void setupMicrosTimer(void);
#define ARDUINO_MAIN
#define NOT_A_PIN 0
#define NOT_A_PORT 0
#define OUTPUT 0
#define INPUT 1
#define INPUT_PULLUP 2
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
// On the ATmega1280, the addresses of some of the port registers are
// greater than 255, so we can't store them in uint8_t's.
extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];

extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
// extern const uint8_t PROGMEM digital_pin_to_bit_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM digital_pin_to_timer_PGM[];
extern const uint8_t PROGMEM analog_pin_to_channel_PGM[];
#define digitalPinToPort(P) (pgm_read_byte(digital_pin_to_port_PGM + (P)))
#define digitalPinToBitMask(P)                                                 \
  (pgm_read_byte(digital_pin_to_bit_mask_PGM + (P)))
#define digitalPinToTimer(P) (pgm_read_byte(digital_pin_to_timer_PGM + (P)))
#define analogInPinToBit(P) (P)
#define portOutputRegister(P)                                                  \
  ((volatile uint8_t *)(pgm_read_word(port_to_output_PGM + (P))))
#define portInputRegister(P)                                                   \
  ((volatile uint8_t *)(pgm_read_word(port_to_input_PGM + (P))))
#define portModeRegister(P)                                                    \
  ((volatile uint8_t *)(pgm_read_word(port_to_mode_PGM + (P))))

#define NOT_AN_INTERRUPT -1
#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)

#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())

#define microsecondsToClockCycles(a) ((a)*clockCyclesPerMicrosecond())
int digitalRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);
void setupADC(void);
void enablePCI(uint8_t pin);
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);