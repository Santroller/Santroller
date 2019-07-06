#include "../../shared/input/input_handler.h"
#include "../../shared/util.h"
#include "../../shared/config/eeprom.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stddef.h>

size_t controller_index = 0;
controller_t controller;
bool done = false;
int main(void) {
  UBRR0 = 8;
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(UDRIE0);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
  // Notify the usb processor so that it knows we are about to wait for the
  // config
  while (!bit_is_set(UCSR0A, RXC0)) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = 0xFE;
  }
  for (size_t i = 0; i < sizeof(config_t); i++) {
    loop_until_bit_is_set(UCSR0A, RXC0);
    ((uint8_t *)&config)[i] = UDR0;
  }
  input_init();
  while (true) {
    input_tick(&controller);
  }
}
ISR(USART_UDRE_vect) {
  if (controller_index < 2) {
    UDR0 = controller_index == 0 ? 'm' : 'a';
  } else {
    UDR0 = ((uint8_t *)&controller)[controller_index - 2];
  }
  controller_index++;
  if (controller_index >= sizeof(controller_t) + 2) {
    controller_index = 0;
  }
}
