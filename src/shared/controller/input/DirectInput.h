#include "../../config/eeprom.h"
#include "../../io/pins/Pins.h"
#include "../Controller.h"
#include "Input.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
class Direct : public Input {
public:
  void read_controller(Controller *controller);
  void init();
};

#define READ_JOY(axis)                                                         \
  controller->axis = config.pins.axis == INVALID_PIN                           \
                         ? 0                                                   \
                         : (IO::analogRead(config.pins.l_x) * 32)

#define DEFINE_JOY(axis)                                                       \
  if (config.pins.axis != INVALID_PIN)                                         \
  IO::pinMode(config.pins.axis, INPUT)
