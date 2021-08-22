#pragma once
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define memcpy_P memcpy
#define PROGMEM
#endif