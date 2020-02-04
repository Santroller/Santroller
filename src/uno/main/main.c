#include "../../shared/config/eeprom.h"
#include "../../shared/input/input_handler.h"
#include "../../shared/output/reports.h"
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
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = 'm';
    while (controller_index < Size) {
      loop_until_bit_is_set(UCSR0A, UDRE0);
      UDR0 = report[controller_index++];
    }
  }
}