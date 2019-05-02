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
int i = 1;
// Jump to bootloader if F_CPU is incorrect.
void check_freq(void) {
  cli();
  wdt_enable(WDTO_15MS);
  _WD_CONTROL_REG = (1 << WDIE) | (1 << WDP2) | (1 << WDP0);
  sei();
  _delay_ms(60);
  // 60 / 15 = 4, since the watchdog timer does not rely on F_CPU, both the
  // delay and the timer should be the same
  if (i != 4) {
    bootloader();
  }
}

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

ISR(WDT_vect) {
  _WD_CONTROL_REG |= (1 << WDIE);
  i++;
  if (i > 4 && !check_serial()) {
    bootloader();
  }
}