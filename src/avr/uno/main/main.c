#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "controller/guitar_includes.h"
#include "device_consts.h"
#include "eeprom/eeprom.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "rf/rf.h"
#include "timer/timer.h"
#include "util/util.h"
#include <LUFA/Drivers/Misc/RingBuffer.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
Controller_t controller;
Controller_t prevController;
uint8_t currentReport[sizeof(USB_Report_Data_t)];
RingBuffer_t in;
RingBuffer_t out;

uint8_t bufIn[USB2USART_BUFLEN];
uint8_t bufOut[USART2USB_BUFLEN];
bool readyForPacket = true;
long lastPoll = 0;
bool isRF = false;
uint8_t deviceType;
uint8_t fullDeviceType;
bool typeIsGuitar;
bool typeIsDrum;
uint8_t inputType;
uint8_t pollRate;
static inline void Serial_InitInterrupt(const uint32_t BaudRate,
                                        const bool DoubleSpeed) {
  UBRR0 =
      (DoubleSpeed ? SERIAL_2X_UBBRVAL(BaudRate) : SERIAL_UBBRVAL(BaudRate));

  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (DoubleSpeed ? (1 << U2X0) : 0);
  UCSR0B = ((1 << TXEN0) | (1 << RXCIE0) | (1 << RXEN0));

  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
}
void writeData(const uint8_t *buf, uint8_t len) {
  for (int i = 0; i < len; i++) { RingBuffer_Insert(&out, *(buf++)); }
  // Enable tx interrupt to push data
  UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0) | _BV(UDRIE0));
}
void initialise(void) {
  Configuration_t config;
  loadConfig(&config);
  fullDeviceType = config.main.subType;
  deviceType = fullDeviceType;
  pollRate = config.main.pollRate;
  inputType = config.main.inputType;
  typeIsDrum = isDrum(fullDeviceType);
  typeIsGuitar = isGuitar(fullDeviceType);
  setupMicrosTimer();
  if (config.rf.rfInEnabled) {
    initRF(false, config.rf.id, generate_crc32());
    isRF = true;
  } else {
    initInputs(&config);
  }
  initReports(&config);
  initLEDs(&config);
}
int main(void) {
  initialise();
  Serial_InitInterrupt(BAUD, true);
  RingBuffer_InitBuffer(&in, bufIn, USB2USART_BUFLEN);
  RingBuffer_InitBuffer(&out, bufOut, USART2USB_BUFLEN);
  sei();
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
    uint8_t count = RingBuffer_GetCount(&in);

    // Check if we have something worth to send
    if (count) {
      // Write all bytes from USART to the USB endpoint
      do {
        uint8_t data = RingBuffer_Remove(&in);
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
          processHIDReadFeatureReport(data, 0, NULL);
          state = 0;
          break;
        } else if (state == 3) {
          cmd = data;
          if (data == COMMAND_WRITE_CONFIG) {
            isConfig = true;
          } else if (data == COMMAND_SET_LEDS) {
            buf = (uint8_t *)&leds;
          } else {
            // Make sure to read the entire packet even if we aren't doing
            // anything with it
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
        if (isRF) {
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
            if (isConfig) {
              uint8_t configBuf[PACKET_SIZE];
              readConfigBlock(offset, configBuf, PACKET_SIZE);
              nrf24_transmitSync(configBuf, PACKET_SIZE);
            } else {
              nrf24_transmitSync(((uint8_t *)&leds) + offset, PACKET_SIZE);
            }
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
    } else if (millis() - lastPoll > pollRate || isRF) {
      if (isRF) {
        tickRFInput((uint8_t *)&controller, sizeof(XInput_Data_t));
      } else {
        tickInputs(&controller);
        tickLEDs(&controller);
      }
      uint8_t size;
      if (memcmp(&prevController, &controller, sizeof(XInput_Data_t)) != 0 &&
          readyForPacket) {
        fillReport(currentReport, &size, &controller);
        lastPoll = millis();
        readyForPacket = false;
        uint8_t done = FRAME_START_WRITE;
        writeData(&done, 1);
        writeData(&size, 1);
        writeData(currentReport, size);
        memcpy(&prevController, &controller, sizeof(XInput_Data_t));
      }
    }
  }
}
// Data being written back to USB after a read
void writeToUSB(const void *const Buffer, uint8_t Length, uint8_t report,
                const void *request) {
  uint8_t done = FRAME_START_WRITE;
  writeData(&done, 1);
  writeData(&Length, 1);
  writeData((uint8_t *)Buffer, Length);
}

// Since the mega has multiple UARTs, alias the usb UART so that we can use the
// same interrupts below
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define USART_RX_vect USART0_RX_vect
#  define USART_UDRE_vect USART0_UDRE_vect
#endif
/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART_RX_vect) { RingBuffer_Insert(&in, UDR0); }

ISR(USART_UDRE_vect) {
  if (RingBuffer_GetCount(&out)) {
    UDR0 = RingBuffer_Remove(&out);
  } else {
    UCSR0B = ((1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0));
  }
}