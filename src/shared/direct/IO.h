#include "../../config/config.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
extern "C" {
extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
#if defined(__AVR_ATmega32U4__)
#define analogPinToChannel(P) (pgm_read_byte(analog_pin_to_channel_PGM + (P)))
extern const uint8_t PROGMEM analog_pin_to_channel_PGM[];
#endif
}
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#define NOT_A_PIN 0
#define NOT_A_PORT 0
#define digitalPinToPort(P) (pgm_read_byte(digital_pin_to_port_PGM + (P)))
#define digitalPinToBitMask(P)                                                 \
  (pgm_read_byte(digital_pin_to_bit_mask_PGM + (P)))
#define portOutputRegister(P)                                                  \
  ((volatile uint8_t *)(pgm_read_word(port_to_output_PGM + (P))))
#define portInputRegister(P)                                                   \
  ((volatile uint8_t *)(pgm_read_word(port_to_input_PGM + (P))))
#define portModeRegister(P)                                                    \
  ((volatile uint8_t *)(pgm_read_word(port_to_mode_PGM + (P))))
#define OUTPUT 0
#define INPUT 1
#define INPUT_PULLUP 2
class IO {
public:
  static int digitalRead(uint8_t pin);
  static int analogRead(uint8_t pin);
  static void pinMode(uint8_t pin, uint8_t mode);
  static void enableADC();
};