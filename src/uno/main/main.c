#include "../../shared/config/eeprom.h"
#include "../../shared/input/input_handler.h"
#include "../../shared/output/reports.h"
#include "../../shared/output/usb/API.h"
#include "../../shared/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>

/** Macro for calculating the baud value from a given baud rate when the \c U2X
 * (double speed) bit is set.
 *
 *  \param[in] Baud  Target serial UART baud rate.
 *
 *  \return Closest UBRR register value for the given UART frequency.
 */
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)
size_t controller_index = 0;
controller_t controller;
uint8_t report[sizeof(output_report_size_t)];
bool done = false;
bool in_config_mode = false;
const char *mcu = MCU;
const char *board = ARDWIINO_BOARD;
const char *version = VERSION;
const char *signature = SIGNATURE;
const char *freq = STR(F_CPU);

extern uint16_t id;
int main(void) {
  load_config();
  UCSR0B = 0;
  UCSR0A = 0;
  UCSR0C = 0;

  UBRR0 = SERIAL_2X_UBBRVAL(115200);

  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (1 << U2X0);
  UCSR0B = ((1 << TXEN0) | (1 << RXEN0));
  UDR0 = config.main.sub_type;
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = config.main.poll_rate;
  input_init();
  while (1) {
    input_tick(&controller);
    uint16_t Size;
    create_report(report, &Size, controller);
    controller_index = 0;
    if (!in_config_mode) {
      if (bit_is_set(UCSR0A, RXC0) && UDR0 == 's') { in_config_mode = true; }
      loop_until_bit_is_set(UCSR0A, UDRE0);
      UDR0 = 'm';
      while (controller_index < Size) {
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = report[controller_index++];
      }
    } else if (bit_is_set(UCSR0A, RXC0)) {
      // TODO: move this crap to serial_handler, and use ifdefs to make it compatible with the micro and CDC
      const uint8_t *buf = NULL;
      switch (UDR0) {
      case COMMAND_READ_INFO:
        loop_until_bit_is_set(UCSR0A, RXC0);
        switch (UDR0) {
        case INFO_MAIN_MCU:
          buf = (uint8_t *)mcu;
          break;
        case INFO_CPU_FREQ:
          buf = (uint8_t *)freq;
          break;
        case INFO_BOARD:
          buf = (uint8_t *)board;
          break;
        case INFO_VERSION:
          buf = (uint8_t *)version;
          break;
        case INFO_SIGNATURE:
          buf = (uint8_t *)signature;
          break;
        case INFO_WII_EXT: {
          char str[4];
          itoa(id, str, 16);
          buf = (uint8_t *)str;
        } break;
        }
      }
      if (buf != NULL) {
        while (*(buf)) {
          loop_until_bit_is_set(UCSR0A, UDRE0);
          UDR0 = *(buf++);
        }
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = '\n';
      }
    }
  }
}