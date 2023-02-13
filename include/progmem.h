#pragma once
#if defined(__AVR__) || defined(_15_)
#include <avr/pgmspace.h>
#else
#ifndef memcpy_P
#define memcpy_P memcpy
#endif
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(x) (*(x))
#endif
#ifndef pgm_read_word
#define pgm_read_word(x) (*(x))
#endif
#ifndef pgm_read_float
#define pgm_read_float(x) (*(x))
#endif
#ifndef PSTR
#define PSTR(STR) STR
#endif
#endif