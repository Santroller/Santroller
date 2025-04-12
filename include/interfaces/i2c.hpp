#include <stdbool.h>
#include <stdint.h>

class I2CMasterInterface {
   public:
    void init();
    bool readFromPointer(uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data);
    bool readFromPointerRepeatedStart(uint8_t address, uint8_t pointer, uint8_t length,
                                      uint8_t *data);
    bool readFromPointerSlow(uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data);
    bool writeSingleToPointer(uint8_t address, uint8_t pointer, uint8_t data);
    bool writeToPointer(uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data);
    virtual bool readFrom(uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop) = 0;

    virtual bool writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) = 0;
};

class I2CSlaveInterface {
   public:
    void init();
    void on_byte_received(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_received_end(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_requested(uint8_t (*handler)(uint8_t addr));
};