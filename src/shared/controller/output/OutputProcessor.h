#include "../Controller.h"
#include "../../../config/config.h"
#if OUTPUT_TYPE == XINPUT
#include "XInputPad.h"
#elif OUTPUT_TYPE == KEYBOARD
#include "Keyboard.h"
#endif
#include "../util.h"
class OutputProcessor {
public:
  void process(Controller *controller);
  void init();
};