#include <avr/interrupt.h>
#include <avr/wdt.h>
void reboot(void) {
  cli();
  wdt_enable(WDTO_15MS);
  for (;;) {}
}