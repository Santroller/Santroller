#include <avr/wdt.h>
#include <avr/interrupt.h>
#define MAGIC_KEY_POS (RAMEND-1)
void bootloader() {
  // close interrupts
  cli();

  // write magic key to ram
  *(uint16_t *)MAGIC_KEY_POS = 0x7777;

  // watchdog reset
  wdt_enable(WDTO_15MS);
  for (;;){}
}