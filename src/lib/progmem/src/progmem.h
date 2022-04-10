#pragma once
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define memcpy_P memcpy
#define PROGMEM
#define pgm_read_byte(x) (*(x))
#define pgm_read_word(x) (*(x))
#define pgm_read_float(x) (*(x))
#define PSTR(STR) STR
#endif