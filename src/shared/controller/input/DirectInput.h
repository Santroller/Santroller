#include "../../config/eeprom.h"
#include "../../io/pins/Pins.h"
#include "../../util.h"
#include "../Controller.h"
#include "Input.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#define READ_JOY(axis)                                                         \
  if (config.pins.axis != INVALID_PIN) {                                       \
    controller->axis = ((IO::analogRead(config.pins.axis) - 512) * 64);        \
  }

#define DEFINE_JOY(axis)                                                       \
  if (config.pins.axis != INVALID_PIN)                                         \
  IO::pinMode(config.pins.axis, INPUT)

class Direct : public Input {
public:
  void read_controller(Controller *controller);
  void init();
};