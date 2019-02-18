#include "../../shared/Controller.h"
#include "../../shared/bootloader/bootloader.h"
#include "../../shared/controller/XInputPad.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <util/delay.h>

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

uint8_t current_control;
ISR(USART1_RX_vect) {
  char data = UDR1;
  switch (current_control) {
  case 0:
    if (data == 'm') {
      current_control++;
    }
    break;
  case 1:
    if (data == 'a')
      current_control++;
    else
      current_control = 0;
    break;
  default:
    ((uint8_t*)&gamepad_state)[current_control-2] = data;
    current_control++;
    if (bit_is_set(gamepad_state.digital_buttons_1, XBOX_START) &&
        bit_is_set(gamepad_state.digital_buttons_1, XBOX_BACK)) {
      bootloader();
    }
    if (current_control == sizeof(USB_JoystickReport_Data_t)) {
      current_control = 0;
    }
    xbox_send_pad_state();
    xbox_reset_watchdog();
  }
}

void USART_Init() {
  // Set baud rate
  UBRR1 = 16;
  // UCSR1A = _BV(U2X1);
  // Enable receiver and interrupt
  UCSR1B = _BV(RXEN1) | _BV(RXCIE1);
  // Set frame format: 8data, 1stop bit
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);
}

int main(void) {

  USART_Init();
  // Set clock @ 16Mhz
  CPU_PRESCALE(0);

  // Init XBOX pad emulation
  xbox_init(true);
  sei();
  for (;;) {
  }
}