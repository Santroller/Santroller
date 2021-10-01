#pragma once
void setupMicrosTimer(void);
unsigned long millis(void);
unsigned long micros(void);
#if __AVR__
extern uint32_t realFreq;
#endif
void delay_ms(uint32_t __ms);
void delay_us(uint32_t __us);