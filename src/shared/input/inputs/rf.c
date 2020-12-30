#include "rf.h"
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include <string.h>

#include "output/controller_structs.h"
#include "pins_arduino.h"
#include "util/util.h"
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <stdint.h>

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
#ifdef __AVR_ATmega32U4__
#  define CE PIN_A0
#  define CSN 10
#else
#  define CE PIN_SPI_SS
#  define CSN 8
#endif
void nrf24_ce_digitalWrite(uint8_t state) { digitalWrite(CE, state); }
void nrf24_csn_digitalWrite(uint8_t state) { digitalWrite(CSN, state); }
uint8_t tx_address[5] = {1,2,3,4, 0xE7};
uint8_t rx_address[5] = {1,2,3,4, 0xD7};
void initRF(bool tx, uint32_t id) {
  rf_interrupt = tx;

  /* init hardware pins */
  pinMode(CE, OUTPUT);
  pinMode(CSN, OUTPUT);
  nrf24_init();

  /* Channel #2 , payload length: 4 */
  nrf24_config(76, sizeof(XInput_Data_t), tx);
  // if (tx) { memcpy(tx_address, &id, sizeof(id)); }
  // id = generate_crc32();
  // memcpy(rx_address, &id, sizeof(id));
  // nrf24_tx_address(tx_address);
  // nrf24_rx_address(rx_address);
  nrf24_tx_address(tx ? tx_address : rx_address);
  nrf24_rx_address(tx ? rx_address : tx_address);
  // on the pro micro, the real ss pin is not accessible, so we should bind it
  // to something else. What if we just use 10 on both?
  // // Micro = int2 = pin 0
  // // Uno = int0 = pin 2
  // // interrupt on falling edge of INT
  // #ifdef __AVR_ATmega32U4__
  //   EICRA |= _BV(ISC21);
  //   EIMSK |= _BV(INT2);
  // #else
  pinMode(2, INPUT_PULLUP);
  EICRA |= _BV(ISC01);
  EIMSK |= _BV(INT0);
  // #endif
  if (tx) { nrf24_send_init(); }
}

bool tickRFTX(Controller_t *controller, uint8_t *data) {
  bool ret = false;
  if (rf_interrupt) {
    rf_interrupt = false;
    if (((nrf24_getStatus() & 0B1110) >> 1) == 0) {
      nrf24_getData((uint8_t *)data);
      ret = true;
    }
    nrf24_send((uint8_t *)controller);
  }
  return ret;
}
uint8_t id = 0;
void tickRFInput(Controller_t *controller) {
  if (rf_interrupt) {
    rf_interrupt = false;
    nrf24_getData((uint8_t *)controller);
  }
}

void writeRFConfig(uint8_t *d) {
  uint8_t data[sizeof(XInput_Data_t)] = {0, 1, 2, id++};
  nrf24_writeAckPayload(data);
}

// #ifdef __AVR_ATmega32U4__
// ISR(INT2_vect) {
// #else
ISR(INT0_vect) {
  // #endif
  rf_interrupt = true;
}
