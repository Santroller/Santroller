#include "../../shared/config/eeprom.h"
#include "../../shared/input/input_handler.h"
#include "../../shared/output/reports.h"
#include "../../shared/output/serial_handler.h"
#include "../../shared/output/usb/API.h"
#include "../../shared/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>
#define FRAME_START_1 0x7c
#define FRAME_START_2 0x7e
#define FRAME_END 0x7f
#define ESC 0x7b
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
/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
RingBuffer_t Buffer;
uint8_t      BufferData[128];

uint8_t read_usb(void) {
  loop_until_bit_is_set(UCSR0A, RXC0);
  return UDR0;
}
bool can_read_usb(void) { return bit_is_set(UCSR0A, RXC0); }

void write_usb(uint8_t data) {
  if (data == FRAME_START_1 || data == FRAME_START_2 || data == ESC ||
      data == FRAME_END) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = ESC;
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = data ^ 0x20;
  } else {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = data;
  }
}
int main(void) {
  load_config();
  init_serial();
  UCSR0B = 0;
  UCSR0A = 0;
  UCSR0C = 0;

  UBRR0 = SERIAL_2X_UBBRVAL(1000000);

  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (1 << U2X0);
  UCSR0B = ((1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0));
  RingBuffer_InitBuffer(&Buffer, BufferData, sizeof(BufferData));
  input_init();
  report_init();
  while (1) {
    input_tick(&controller);
    uint16_t Size;
    create_report(report, &Size, controller);
    controller_index = 0;
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = FRAME_START_1;
    while (controller_index < Size) { write_usb(report[controller_index++]); }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = FRAME_START_2;
    for (int i =0; i < RingBuffer_GetCount(&Buffer); i++) {
      process_serial(RingBuffer_Remove(&Buffer));
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = FRAME_END;
  }
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR0;
    RingBuffer_Insert(&Buffer, ReceivedByte);
}