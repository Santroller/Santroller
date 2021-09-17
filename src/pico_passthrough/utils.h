#pragma once
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
// #define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_write(c, p, m) ((p) = (p & ~(_BV(m))) | ((!!(c)) << (m)));
#define bit_check(value, bit) (value & _BV(bit))
#define PRINTF_BINARY_PATTERN_INT8 "%c%c, %c%c, %c%c, %c%c, "
#define PRINTF_BYTE_TO_BINARY_INT8(i) \
    (((i)&0x01ll) ? '1' : '0'),       \
        (((i)&0x02ll) ? '1' : '0'),   \
        (((i)&0x04ll) ? '1' : '0'),   \
        (((i)&0x08ll) ? '1' : '0'),   \
        (((i)&0x10ll) ? '1' : '0'),   \
        (((i)&0x20ll) ? '1' : '0'),   \
        (((i)&0x40ll) ? '1' : '0'),   \
        (((i)&0x80ll) ? '1' : '0')
