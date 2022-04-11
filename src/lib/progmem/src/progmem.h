#pragma once
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define memcpy_P memcpy
#define PROGMEM
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