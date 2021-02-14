#include <stdint.h>
#include "controller/controller.h"
#include <stdbool.h>
void initRF(bool tx, uint32_t txid, uint32_t rxid);
uint8_t tickRFInput(uint8_t *controller, uint8_t len);
int tickRFTX(uint8_t *data2, uint8_t* data, uint8_t len);
uint32_t generate_crc32(void);
extern volatile bool rf_interrupt;

extern bool p_type;
extern bool wide_band;