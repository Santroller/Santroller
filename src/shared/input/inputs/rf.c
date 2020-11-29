#include "controller/controller.h"
#include "rf.h"
#include "avr-nrf24l01/src/nrf24l01.h"

#include <avr/boot.h>
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

nRF24L01 *rf;
void initRFInput(Controller_t *controller) {
//   rf = setup_rf();
//   nRF24L01_listen(rf, 0, generate_crc32());
}

void tickRFInput(Controller_t *controller) {}