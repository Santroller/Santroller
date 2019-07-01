#include "../../shared/controller/input/InputHandler.h"
#include "../../shared/io/uart/uart.h"
#include "../../shared/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <util/delay.h>

InputHandler controller;

size_t controller_index = 0;
size_t config_index = 0;
bool done = false;
int main() {
  UBRR0 = 6;
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(UDRIE0);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
  controller.init();
  sei();
  while (true) {
    controller.process();
    bit_set(UCSR0B, UDRIE0);
  }
}
ISR(USART_UDRE_vect) {
  if (controller_index < 2) {
    UDR0 = controller_index == 0 ? 'm' : 'a';
  } else {
    UDR0 = ((uint8_t *)&controller.controller)[controller_index - 2];
  }
  controller_index++;
  if (controller_index >= sizeof(Controller) + 2) {
    controller_index = 0;
    bit_clear(UCSR0B, UDRIE0);
  }
}
ISR(USART_RX_vect) {
  ((uint8_t *)&config)[config_index] = UDR0;
  config_index++;
  if (config_index >= sizeof(config_t)) {
    cli();
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
  }
}
void configChangeHandler() {}
