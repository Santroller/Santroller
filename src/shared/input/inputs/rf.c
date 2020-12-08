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
#include <stdint.h>

bool write_config;
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
void nrf24_ce_digitalWrite(uint8_t state) { digitalWrite(8, state); }
void nrf24_csn_digitalWrite(uint8_t state) { digitalWrite(PIN_SPI_SS, state); }
uint8_t tx_address[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
uint8_t rx_address[5] = {0xD7, 0xD7, 0xD7, 0xD7, 0xD7};
void initRF(bool tx) {
  /* init hardware pins */
  nrf24_init();

  /* Channel #2 , payload length: 4 */
  nrf24_config(2, sizeof(XInput_Data_t));
  nrf24_tx_address(tx ? tx_address : rx_address);
  nrf24_rx_address(tx ? rx_address : tx_address);
  // on the pro micro, the real ss pin is not accessible, so we should bind it
  // to something else. What if we just use 10 on both?
}

// nRF24L01Message msg;
void tickRFTX(Controller_t *controller) {
  /* Automatically goes to TX mode */
  nrf24_send((uint8_t *)controller);

  /* Wait for transmission to end */
  while (nrf24_isSending())
    ;

  nrf24_powerUpRx();
  _delay_ms(10);
}

void tickRFInput(Controller_t *controller) {
  if (nrf24_dataReady()) {
    nrf24_getData((uint8_t *)controller);
  }
}
