#pragma once
void setupMicrosTimer(void);
unsigned long millis(void);
unsigned long micros(void);
#if __AVR__
#  include <util/delay.h>
#else
#  include <hardware/timer.h>
#  include <pico/stdlib.h>
#  include <pico/time.h>
static __inline__ void _delay_us(double __us)
    __attribute__((__always_inline__));
static __inline__ void _delay_ms(double __ms)
    __attribute__((__always_inline__));
void _delay_ms(double __ms) { sleep_ms(__ms); }
void _delay_us(double __us) { busy_wait_us_32(__us); }
#endif