#include "../../shared/config/eeprom.h"
#include "../../shared/output/output_handler.h"
#include "../../shared/util.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <util/delay.h>
#include "../../shared/output/output_serial.h"

uint8_t controller_index;
controller_t controller;
ISR(USART1_RX_vect, ISR_BLOCK) {
  char data = UDR1;
  switch (controller_index) {
  case 0:
    if (data == 'm') { controller_index++; }
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
    if (controller_index >= sizeof(controller_t) + 2) {
      output_tick(controller);
      controller_index = 0;
    }
  }
  serial_receive(data);
}
int main(void) {
  load_config();
  serial_init();
  uint8_t *cfg = (uint8_t *)&config;
  for (size_t i = 0; i < sizeof(config_t); i++) {
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = cfg[i];
  }
  output_init();
  // clang-format off
  while (true) {
    serial_tick();
  }
  // clang-format on
}
void before_reset(void) {
  loop_until_bit_is_set(UCSR1A, UDRE1);
  UDR1 = 'r';
}