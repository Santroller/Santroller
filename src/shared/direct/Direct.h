#include "../../config/config.h"
#include "../controller/Controller.h"
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "IO.h"
class Direct {
public:
  void read_controller(Controller *controller);
  void init();
};