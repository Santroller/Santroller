#include <stdint.h>
#include "controller/controller.h"
#include <stdbool.h>
void initRFInput(void);
void initRF(bool tx);
void tickRFInput(Controller_t *controller);
bool tickRFTX(Controller_t *controller, uint8_t* data);
uint32_t generate_crc32(void);
void tickRFTXIRQ(void);
void writeRFConfig(uint8_t* data);
extern bool rf_interrupt;