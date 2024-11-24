#include "io.h"
#include <string.h>
bool twi_readFromPointer(TWI_BLOCK block, uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data) {
  return twi_writeTo(block, address, &pointer, 1, true, true) &&
         twi_readFrom(block, address, data, length, true);
}
bool twi_readFromPointerRepeatedStart(TWI_BLOCK block, uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data) {
  return twi_writeTo(block, address, &pointer, 1, true, false) &&
         twi_readFrom(block, address, data, length, true);
}
bool twi_writeSingleToPointer(TWI_BLOCK block, uint8_t address, uint8_t pointer, uint8_t data) {
  return twi_writeToPointer(block, address, pointer, 1, &data);
}
bool twi_writeToPointer(TWI_BLOCK block, uint8_t address, uint8_t pointer, uint8_t length,
                        const uint8_t *data) {
  uint8_t data2[length + 1];
  data2[0] = pointer;
  memcpy(data2 + 1, data, length);

  return twi_writeTo(block, address, data2, length + 1, true, true);
}