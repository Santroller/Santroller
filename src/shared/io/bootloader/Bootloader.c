#include "Bootloader.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#ifdef __AVR_ATmega32U4__
#define MAGIC_KEY_POS 0x0800
#else
#define MAGIC_KEY_POS (RAMEND - 1)
#endif
#define MAGIC_KEY 0x7777
volatile uint16_t *const bootKeyPtr = (volatile uint16_t *)MAGIC_KEY_POS;
volatile uint16_t *const bootKeyPtrCDC = (volatile uint16_t *)(MAGIC_KEY_POS-2);
int i = 1;
void bootloader(void) {
  // close interrupts
  cli();

  // write magic key to ram
  *bootKeyPtr = MAGIC_KEY;

  // watchdog reset
  wdt_enable(WDTO_15MS);
  for (;;) {
  }
}

void check_freq(void) {
  cli();
  wdt_enable(WDTO_15MS);
  _WD_CONTROL_REG = (1<<WDIE) | (1<<WDP2) | (1<<WDP0);
  sei();
  _delay_ms(60);
  if (i != 4) {
    bootloader();
  }
  wdt_disable();
}

void serial(void) {
  // close interrupts
  cli();

  // write magic key to ram
  *bootKeyPtrCDC = MAGIC_KEY;

  // watchdog reset
  wdt_enable(WDTO_15MS);
  for (;;) {
  }
}

bool check_serial() { 
  uint16_t bootKeyVal = *bootKeyPtrCDC;
  *bootKeyPtrCDC = 0;
  return bootKeyVal == MAGIC_KEY; 
}

ISR(WDT_vect)        // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  _WD_CONTROL_REG |= (1<<WDIE);
  i++;
}