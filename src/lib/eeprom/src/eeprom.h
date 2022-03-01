#pragma once
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define memcpy_P memcpy
#define PROGMEM
#endif
#ifdef __AVR__
#  define AVR_CONST const PROGMEM
#else
#  define AVR_CONST
#endif