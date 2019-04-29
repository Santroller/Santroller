#include "sharedmain.h"
#include "io/bootloader/Bootloader.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
void Main::main() {
  check_freq();
}