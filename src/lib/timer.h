#pragma once
void setupMicrosTimer(void);
unsigned long millis(void);
unsigned long micros(void);
#if __AVR__
#  include <util/delay.h>
#else
void _delay_ms(uint32_t __ms);
void _delay_us(uint32_t __us);
#endif