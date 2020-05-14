#include "../../shared/config/eeprom.h"
#include "../../shared/input/input_handler.h"
#include "../../shared/output/reports.h"
#include "../../shared/output/serial_handler.h"
#include "../../shared/output/usb/API.h"
#include "../../shared/util.h"
#include <LUFA/Drivers/Misc/RingBuffer.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
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
uint8_t prev_report[sizeof(output_report_size_t)];
/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
RingBuffer_t Buffer;
uint8_t BufferData[128];
RingBuffer_t BufferOut;
uint8_t BufferOutData[128];

void write_usb(uint8_t data) {
  if (data == FRAME_START_1 || data == FRAME_START_2 || data == ESC ||
      data == FRAME_END) {
    RingBuffer_Insert(&BufferOut, ESC);
    loop_until_bit_is_set(UCSR0A, UDRE0);
    data = data ^ 0x20;
  }
  RingBuffer_Insert(&BufferOut, data);
}
uint8_t detected_pin;
bool found_pin = false;
int main(void) {
  load_config();
  UCSR0B = 0;
  UCSR0A = 0;
  UCSR0C = 0;

  UBRR0 = SERIAL_2X_UBBRVAL(1000000);
  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (1 << U2X0);
  UCSR0B = ((1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0));
  RingBuffer_InitBuffer(&Buffer, BufferData, sizeof(BufferData));
  RingBuffer_InitBuffer(&BufferOut, BufferOutData, sizeof(BufferOutData));
  input_init();
  report_init();
  while (1) {
    input_tick(&controller);
    uint16_t size;
    create_report(report, &size, &controller);
    if (memcmp(report, prev_report, size) != 0) {
      controller_index = 0;
      RingBuffer_Insert(&BufferOut, FRAME_START_1);
      while (controller_index < size) {
        RingBuffer_Insert(&BufferOut, report[controller_index++]);
      }
      RingBuffer_Insert(&BufferOut, FRAME_END);
      memcpy(prev_report, report, size);
    }

    if (found_pin) {
      found_pin = false;
      RingBuffer_Insert(&BufferOut, FRAME_START_2);
      write_usb('d');
      write_usb(detected_pin);
      write_usb('\r');
      write_usb('\n');
      RingBuffer_Insert(&BufferOut, FRAME_END);
      RingBuffer_Insert(&BufferOut, FRAME_END);
    }
    size = RingBuffer_GetCount(&Buffer);
    if (size != 0) {
      RingBuffer_Insert(&BufferOut, FRAME_START_2);
      for (int i = 0; i < RingBuffer_GetCount(&Buffer); i++) {
        process_serial(RingBuffer_Remove(&Buffer));
      }
      RingBuffer_Insert(&BufferOut, FRAME_END);
    }
    size = RingBuffer_GetCount(&BufferOut);
    while (size--) {
      loop_until_bit_is_set(UCSR0A, UDRE0);
      UDR0 = RingBuffer_Remove(&BufferOut);
    }
  }
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR0;
  RingBuffer_Insert(&Buffer, ReceivedByte);
}
/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART_UDRE_vect, ISR_BLOCK) {
  if (!RingBuffer_IsEmpty(&BufferOut)) { UDR0 = RingBuffer_Remove(&BufferOut); }
}