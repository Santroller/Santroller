#include "../../shared/config/eeprom.h"
#include "../../shared/output/output_handler.h"
#include "../../shared/output/output_serial.h"
#include "../../shared/util.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <util/delay.h>
#include <LUFA/Drivers/Board/Board.h>

uint8_t controller_index;
controller_t controller;
int main(void) {
  //Reset the main MPU when this MPU boots
  AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
  load_config();
  UBRR1 = 16;
  UCSR1A = (1 << U2X1);
  UCSR1B = _BV(TXEN1) | _BV(RXEN1) | _BV(RXCIE1);
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);
  uint8_t data = 0;
  while (data != 0xFE) {
    loop_until_bit_is_set(UCSR1A, RXC1);
    data = UDR1;
  }
  uint8_t *cfg = (uint8_t *)&config;
  for (size_t i = 0; i < sizeof(config_t); i++) {
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = cfg[i];
  }
  output_init();
  serial_init(&controller);
  while (true) {
    serial_tick();
  }
}