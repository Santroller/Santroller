#include "Bootloader.h"
#include "../../util.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#ifdef __AVR_ATmega32U4__
#  define MAGIC_KEY_POS 0x0800
#else
#  define MAGIC_KEY_POS (RAMEND - 1)
#endif
#define MAGIC_KEY 0x7777
extern void before_reboot(void);
volatile uint16_t *const bootKeyPtr = (volatile uint16_t *)MAGIC_KEY_POS;
volatile uint16_t CDCJumpKey __attribute__((section(".noinit")));
uint16_t bootKeyVal;

void serial_jump_init() {
  bootKeyVal = CDCJumpKey;
  CDCJumpKey = 0;
}

void reboot(void) {
  cli();
  wdt_enable(WDTO_15MS);
  before_reboot();
  for (;;) {
  }
}

void serial(void) {
  // write magic key to ram
  CDCJumpKey = MAGIC_KEY;

  reboot();
}

void bootloader(void) {
  // write magic key to ram
  *bootKeyPtr = MAGIC_KEY;

  reboot();
}

bool check_serial() { return bootKeyVal == MAGIC_KEY; }