#pragma once
#include <avr/wdt.h>
void bootloader() {
  uint16_t *const bootKeyPtr = (uint16_t *)0x0800;

  // Value used by Caterina bootloader use to determine whether to run the
  // sketch or the bootloader programmer.
  uint16_t bootKey = 0x7777;

  *bootKeyPtr = bootKey;

  // setup watchdog timeout
  wdt_enable(WDTO_15MS);

  while(1) {} // wait for watchdog timer to trigger
}
