#include <stdint.h>
#include <stdbool.h>

class PS2OutputInterface {
   public:
    void init();
    uint8_t transfer(uint8_t data);
    uint8_t transfer_with_timeout(uint8_t data);
};