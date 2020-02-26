#pragma once
#include <avr/sfr_defs.h>

/** wchar_t is not available on all platforms */
#if ((ARCH == ARCH_AVR8) || (ARCH == ARCH_XMEGA))
#  define WCHAR wchar_t
#else
#  define WCHAR uint16_t
#endif
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