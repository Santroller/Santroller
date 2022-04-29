#include "i2c.h"
#include "timer/timer.h"
#include "util/util.h"
bool twi_readFromPointerSlow(uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data) {
  if (!twi_writeTo(address, &pointer, 1, true, true)) return false;
  _delay_us(175);
  return twi_readFrom(address, data, length, true);
}
bool twi_readFromPointer(uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data) {
  return twi_writeTo(address, &pointer, 1, true, true) &&
         twi_readFrom(address, data, length, true);
}
bool twi_writeSingleToPointer(uint8_t address, uint8_t pointer, uint8_t data) {
  return twi_writeToPointer(address, pointer, 1, &data);
}
bool twi_writeToPointer(uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data) {
  uint8_t data2[length + 1];
  data2[0] = pointer;
  memcpy(data2 + 1, data, length);

  return twi_writeTo(address, data2, length + 1, true, true);
}