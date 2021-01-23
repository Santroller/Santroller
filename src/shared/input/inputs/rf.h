#include <stdint.h>
#include "controller/controller.h"
#include <stdbool.h>
void initRF(bool tx, uint32_t txid, uint32_t rxid);
bool tickRFInput(Controller_t *controller);
bool tickRFTX(Controller_t *controller, uint8_t* data);
uint32_t generate_crc32(void);
extern volatile bool rf_interrupt;

extern bool p_type;
extern bool wide_band;