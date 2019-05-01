#include "sharedmain.h"
#include "io/bootloader/Bootloader.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "config/eeprom.h"
void Main::main() {
  eeprom_read_block(&config, &config_pointer, sizeof(config_t));
  check_freq();
}