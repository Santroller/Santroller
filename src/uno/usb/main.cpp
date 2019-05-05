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

uint8_t controller_index;
uint8_t config_index;
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
  sharedMain.main();
  UBRR1 = 6;
  UCSR1B = _BV(TXEN1) | _BV(RXEN1) | _BV(RXCIE1);
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);
  uint8_t data = 0;
  // Wait for the main processor to notify us that it is about to wait for data.
  while (data != 0xFE) {
    loop_until_bit_is_set(UCSR1A, RXC1);
    data = UDR1;
  }
  for (size_t i = 0; i < sizeof(config_t); i++) {
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = ((uint8_t *)&config)[i];
  }
  out.init();
  sei();
  // clang-format off
  while (true);
  // clang-format on
}