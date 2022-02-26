#pragma once
#include <stdbool.h>
#include <stdint.h>
void setupMicrosTimer(void);
unsigned long millis(void);
unsigned long micros(void);
#if __AVR__
extern uint32_t realFreq;
extern bool scaleDiv2;
extern bool scaleMul2;
#endif
void delay_ms(uint32_t __ms);
void delay_us(uint32_t __us);