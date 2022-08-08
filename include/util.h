#ifndef cbi
#  define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#  define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#ifndef _BV
#  define _BV(bit) (1 << (bit))
#endif
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
// #define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_write(c, p, m) ((p) = (p & ~(_BV(m))) | ((!!(c)) << m))
#define bit_check(value, bit) (value & _BV(bit))