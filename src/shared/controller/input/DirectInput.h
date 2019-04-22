#include "../../../config/config.h"
#include "../Controller.h"
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "../../io/pins/Pins.h"
class Direct {
public:
  void read_controller(Controller *controller);
  void init();
};