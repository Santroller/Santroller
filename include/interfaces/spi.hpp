#include <stdint.h>
#include <stdbool.h>

class SPIMasterInterface {
   public:
    void init();
    void begin();
    uint8_t transfer(uint8_t data);
};