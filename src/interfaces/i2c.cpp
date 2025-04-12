#include <stdint.h>
#include <interfaces/core.hpp>
#include <interfaces/i2c.hpp>
#include <string.h>

bool I2CMasterInterface::readFromPointerSlow(uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data) {
    if (!writeTo(address, &pointer, 1, true, true)) return false;
    Core::delayMicroseconds(170);
    return readFrom(address, data, length, true);
}

bool I2CMasterInterface::readFromPointer(uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data) {
  return writeTo(address, &pointer, 1, true, true) &&
         readFrom(address, data, length, true);
}
bool I2CMasterInterface::readFromPointerRepeatedStart(uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data) {
  return writeTo(address, &pointer, 1, true, false) &&
         readFrom(address, data, length, true);
}
bool I2CMasterInterface::writeSingleToPointer(uint8_t address, uint8_t pointer, uint8_t data) {
  return writeToPointer(address, pointer, 1, &data);
}
bool I2CMasterInterface::writeToPointer(uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data) {
  uint8_t data2[length + 1];
  data2[0] = pointer;
  memcpy(data2 + 1, data, length);

  return writeTo(address, data2, length + 1, true, true);
}