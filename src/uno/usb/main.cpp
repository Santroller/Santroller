#include "../../shared/controller/Controller.h"
#include "../../shared/controller/output/OutputHandler.h"
#include "../../shared/io/bootloader/Bootloader.h"
#include "../../shared/sharedmain.h"
#include "../../shared/util.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <util/delay.h>

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

uint8_t controller_index;
uint8_t config_index;
volatile uint16_t restart_test __attribute__((section(".noinit")));
Controller controller;
OutputHandler out;
Main sharedMain;
ISR(USART1_RX_vect) {
  char data = UDR1;
  switch (controller_index) {
  case 0:
    if (data == 'm') {
      controller_index++;
    }
    break;
  case 1:
    if (data == 'a')
      controller_index++;
    else
      controller_index = 0;
    break;
  default:
    ((uint8_t *)&controller)[controller_index - 2] = data;
    controller_index++;
    if (controller_index >= sizeof(Controller) + 2) {
      out.process(&controller);
      controller_index = 0;
    }
  }
}
int main(void) {
  // TODO: why is it that restarting the whole thing makes things sync up
  // correctly?
  if (restart_test != 0x5678) {
    restart_test = 0x5678;
    USB_Detach();
    wdt_enable(WDTO_250MS);
    // clang-format off
    while (true);
    // clang-format on
  }
  CPU_PRESCALE(0);
  sharedMain.main();
  UBRR1 = 6;
  UCSR1B = _BV(TXEN1) | _BV(RXEN1) | _BV(RXCIE1);
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);
  for (size_t i = 0; i < sizeof(config_t); i++) {
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = ((uint8_t *)&config)[i];
  }
  out.init();
  sei();
  while (true) {
  };
}