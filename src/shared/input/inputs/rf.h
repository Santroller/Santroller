#include <stdint.h>
#include "controller/controller.h"
#include <stdbool.h>
void initRFInput(void);
void initRF(bool tx, uint32_t id);
void tickRFInput(Controller_t *controller);
bool tickRFTX(Controller_t *controller, uint8_t* data);
uint32_t generate_crc32(void);
void tickRFTXIRQ(void);
void writeRFConfig(uint8_t* data);
extern volatile bool rf_interrupt;

extern bool p_type;
extern bool wide_band;