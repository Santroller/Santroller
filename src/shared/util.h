#pragma once
#include <avr/sfr_defs.h>

#define bit_set(p,m) ((p) |= _BV(m))
#define bit_clear(p,m) ((p) &= ~_BV(m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define bit_check(value, bit) (value & _BV(bit))

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))