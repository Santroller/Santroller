#include "../shared/config/eeprom.h"
#include "../shared/input/input_handler.h"
#include "../shared/output/output_handler.h"
#include "stdbool.h"
controller_t controller;
int main(void) {
  load_config();
  input_init();
  output_init();
  while (true) {
    input_tick(&controller);
    output_tick(controller);
    serial_tick();
  }
}
void before_reset(void) {}