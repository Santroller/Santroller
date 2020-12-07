#include "rf.h"
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include <string.h>

#include "arduino_pins.h"
#include "output/controller_structs.h"
#include "util/util.h"
#include <avr/boot.h>
#include <avr/interrupt.h>

nRF24L01 *rf;
bool write_config;
volatile bool rf_interrupt = false;
uint32_t generate_crc32(void) {
  uint32_t crc = 0x01234567;
  int i, j;
  for (i = 0; i < 32; i++) {
    crc = crc ^ boot_signature_byte_get(i);
    for (j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc = crc >> 1;
    }
  }
  return crc;
}
void initRF(void) {
  rf = nRF24L01_init();
  // on the pro micro, the real ss pin is not accessible, so we should bind it
  // to something else. What if we just use 10 on both?
  rf->ss.port = portOutputRegister(digitalPinToPort(PIN_SPI_SS));
  rf->ss.pin = digitalPinToBitMask(PIN_SPI_SS);
  rf->ce.port = portOutputRegister(digitalPinToPort(8));
  rf->ce.pin = digitalPinToBitMask(8);
  rf->sck.port = portOutputRegister(digitalPinToPort(PIN_SPI_SCK));
  rf->sck.pin = digitalPinToBitMask(PIN_SPI_SCK);
  rf->mosi.port = portOutputRegister(digitalPinToPort(PIN_SPI_MOSI));
  rf->mosi.pin = digitalPinToBitMask(PIN_SPI_MOSI);
  rf->miso.port = portOutputRegister(digitalPinToPort(PIN_SPI_MISO));
  rf->miso.pin = digitalPinToBitMask(PIN_SPI_MISO);
// Micro = int2 = pin 0
// Uno = int0 = pin 2
// interrupt on falling edge of INT
#ifdef __AVR_ATmega32U4__
  EICRA |= _BV(ISC21);
  EIMSK |= _BV(INT2);
#else
  EICRA |= _BV(ISC01);
  EIMSK |= _BV(INT0);
#endif
  nRF24L01_begin(rf);
}
void initRFInput(void) {
  initRF();
  uint8_t addr[5] = {0x01, 0x01, 0x01, 0x01, 0x01};
  nRF24L01_listen(rf, 0, addr);
  uint8_t addr2[5];
  nRF24L01_read_register(rf, CONFIG, addr2, 1);
  // for (int i = 0; i < 4; i++) {
  //   if (config.rf.rfAddresses[i]) {
  //     uint8_t addr[5];
  //     memcpy(addr, &config.rf.rfAddresses[i],
  //     sizeof(config.rf.rfAddresses[i]));
  //     nRF24L01_listen(rf, i, addr);
  //   }
  // }
}

nRF24L01Message msg;
void tickRFTX(Controller_t *controller) {
  uint8_t to_address[5] = {0x01, 0x01, 0x01, 0x01, 0x01};
  memcpy(&msg.data, controller, sizeof(XInput_Data_t));
  nRF24L01_transmit(rf, to_address, &msg);
}

void tickRFTXIRQ(void) {
  if (rf_interrupt) {
    rf_interrupt = false;
    int success = nRF24L01_transmit_success(rf);
    if (success != 0) nRF24L01_flush_transmit_message(rf);
  }
}

void tickRFInput(Controller_t *controller) {
  if (rf_interrupt) {
    rf_interrupt = false;
    while (nRF24L01_data_received(rf)) {
      nRF24L01_read_received_data(rf, &msg);
      memcpy(controller, &msg.data, sizeof(XInput_Data_t));
    }
    uint8_t addr[5] = {0x01, 0x01, 0x01, 0x01, 0x01};
    nRF24L01_listen(rf, 0, addr);
  }
}

#ifdef __AVR_ATmega32U4__
ISR(INT2_vect) {
#else
ISR(INT0_vect) {
#endif
  rf_interrupt = true;
}
