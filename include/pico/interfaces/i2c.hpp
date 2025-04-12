#include <hardware/i2c.h>
#include <stdbool.h>
#include <stdint.h>

#include <interfaces/i2c.hpp>

class PicoI2CMasterInterface : I2CMasterInterface {
   public:
    bool readFrom(uint8_t address, uint8_t *data, uint8_t length,
                          uint8_t sendStop);

    bool writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                         uint8_t sendStop);

   private:
    i2c_inst_t *i2c;
};