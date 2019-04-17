#include "../../shared/controller/InputHandler.h"
#include "../../shared/twi/I2Cdev.h"
#include "../../shared/util.h"
#include "../../shared/wii/WiiExtension.h"
#include "uart.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
extern "C" {
#include "pins.h"
}

InputHandler controller;

size_t current_index = 0;
int main() {
  sei();
  uart_init();
  controller.init();
  UCSR0B = _BV(TXEN0) | _BV(UDRIE0);
  while(true) {
    controller.process();
  }
}
ISR(USART_UDRE_vect) {
  if (current_index < 2) {
    UDR0 = current_index == 0 ? 'm' : 'a';
  } else {
    UDR0 = ((uint8_t *)&controller.controller)[current_index - 2];
  }
  current_index++;
  if (current_index >= sizeof(Controller) + 2) {
    current_index = 0;
  }
}
