#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "eeprom/eeprom.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "device_consts.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "rf/rf.h"
#include "timer/timer.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>
Controller_t controller;
uint8_t currentReport[sizeof(USB_Report_Data_t)];
uint8_t previousReport[sizeof(USB_Report_Data_t)];
RingBuffer_t USB2USART;
RingBuffer_t USART2USB;
uint8_t USB2USART_BUF[USB2USART_BUFLEN];
uint8_t USART2USB_BUF[USART2USB_BUFLEN];
bool readyForPacket = true;
long lastPoll = 0;
static inline void Serial_InitInterrupt(const uint32_t BaudRate,
                                        const bool DoubleSpeed) {
  UBRR0 = (DoubleSpeed ? SERIAL_2X_UBBRVAL(BaudRate) : SERIAL_UBBRVAL(BaudRate));

  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (DoubleSpeed ? (1 << U2X0) : 0);
  UCSR0B = ((1 << TXEN0) | (1 << RXCIE0) | (1 << RXEN0));

  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
}
void writeData(const uint8_t *buf, uint8_t len) {
  for (int i = 0; i < len ; i++) {
    RingBuffer_Insert(&USB2USART, *(buf++));
  }
  UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0) | _BV(UDRIE0));
}
int main(void) {
  loadConfig();
  Serial_InitInterrupt(BAUD, true);
  RingBuffer_InitBuffer(&USB2USART, USB2USART_BUF, USB2USART_BUFLEN);
  RingBuffer_InitBuffer(&USART2USB, USART2USB_BUF, USART2USB_BUFLEN);
  sei();
  setupMicrosTimer();
  if (config.rf.rfInEnabled) {
    initRF(false, config.rf.id, generate_crc32());
  } else {
    initInputs();
  }
  initReports();
  uint8_t packetCount = 0;
  uint8_t state = 0;
  uint8_t *buf;
  uint8_t cmd;
  bool isConfig = false;
  uint16_t offset;
  uint8_t origOffset;
  while (true) {
    //================================================================================
    // USARTtoUSB
    //================================================================================

    // This requires the USART RX buffer to be 256 bytes.
    uint8_t count = RingBuffer_GetCount(&USART2USB);

    // Check if we have something worth to send
    if (count) {
      // Write all bytes from USART to the USB endpoint
      do {
        uint8_t data = RingBuffer_Remove(&USART2USB);
        if (state == 0) {
          if (data == FRAME_START_FEATURE_WRITE) {
            state = 1;
          } else if (data == FRAME_START_FEATURE_READ) {
            state = 2;
          } else if (data == FRAME_DONE) {
            readyForPacket = true;
          }
        } else if (state == 1) {
          packetCount = data;
          state = 3;
        } else if (state == 2) {
          processHIDReadFeatureReport(data);
          state = 0;
          break;
        } else if (state == 3) {
          cmd = data;
          if (data == COMMAND_WRITE_CONFIG) {
            isConfig = true;
          } else if (data == COMMAND_SET_LEDS) {
            buf = (uint8_t *)&controller.leds;
          } else {
            // Make sure to read the entire packet even if we aren't doing anything with it
            buf = NULL;
            state = 5;
            continue;
          }
          state = 4;
          packetCount--;
        } else if (state == 4) {
          origOffset = data;
          offset = origOffset * PACKET_SIZE;
          buf += offset;
          state = 5;
        } else if (state == 5) {
          packetCount--;
          if (isConfig) {
            writeConfigByte(offset++, data);
          } else if (buf) {
            *(buf++) = data;
          } else if (cmd == COMMAND_SET_SP) {
            setSP(data);
          }
          if (packetCount == 0) {
            state = 7;
            break;
          }
        }
      } while (--count);
      if (state == 7) {
        state = 0;
        if (config.rf.rfInEnabled) {
          while (!nrf24_txFifoEmpty()) {
            rf_interrupt = true;
            tickRFInput(buf, 0);
            nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
          }
          nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
          nrf24_csn_digitalWrite(LOW);
          spi_transfer(W_ACK_PAYLOAD | 1);
          spi_transfer(cmd);
          // reading (0 for false, as we are writing)
          spi_transfer(false);
          if (cmd == COMMAND_SET_LEDS || cmd == COMMAND_WRITE_CONFIG) {
            spi_transfer(origOffset);
            nrf24_transmitSync(
                (isConfig ? (uint8_t *)&config : (uint8_t *)&controller.leds) +
                    offset,
                PACKET_SIZE);
          }
          nrf24_csn_digitalWrite(HIGH);
          isConfig = false;
          rf_interrupt = true;
          while (!nrf24_txFifoEmpty()) {
            rf_interrupt = true;
            tickRFInput(buf, 0);
            nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
          }
        }
        if (cmd == COMMAND_REBOOT) { _delay_ms(100); }
        handleCommand(cmd);
      }
      // With RF, this stuff gets handled on the transmitter side, not the
      // receiver.
    } else if (millis() - lastPoll > config.main.pollRate ||
               config.rf.rfInEnabled) {
      if (config.rf.rfInEnabled) {
        tickRFInput((uint8_t *)&controller, sizeof(XInput_Data_t));
      } else {
        tickInputs(&controller);
        tickLEDs(&controller);
      }
      uint8_t size;
      fillReport(currentReport, &size, &controller);
      if (memcmp(currentReport, previousReport, size) != 0 && readyForPacket) {
        lastPoll = millis();
        readyForPacket = false;
        uint8_t done = FRAME_START_WRITE;
        writeData(&done, 1);
        writeData(&size, 1);
        writeData(currentReport, size);
        memcpy(previousReport, currentReport, size);
      }
    }
  }
}
// Data being written back to USB after a read
void writeToUSB(const void *const Buffer, uint8_t Length) {
  uint8_t done = FRAME_START_WRITE;
  writeData(&done, 1);
  writeData(&Length, 1);
  writeData((uint8_t *)Buffer, Length);
}
#  if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#    define USART_RX_vect USART0_RX_vect
#    define USART_UDRE_vect USART0_UDRE_vect
#  endif
/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART_RX_vect) {
  RingBuffer_Insert(&USART2USB, UDR0);
}

ISR(USART_UDRE_vect) {
  if (RingBuffer_GetCount(&USB2USART)) {
    UDR0 = RingBuffer_Remove(&USB2USART);
  } else {
    UCSR0B = ((1<<RXCIE0) | (1 << RXEN0) | (1 << TXEN0));
  }
}