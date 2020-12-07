#include <stdint.h>
#include "controller/controller.h"
#include <stdbool.h>
void initRFInput(void);
void initRF(void);
void tickRFInput(Controller_t *controller);
void tickRFTX(Controller_t *controller);
uint32_t generate_crc32(void);
volatile extern bool rf_interrupt;
void tickRFTXIRQ(void);