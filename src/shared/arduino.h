#pragma once
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