#include "../Controller.h"
#include "../../../config/config.h"
#include "../../io/pins/Pins.h"
#if DEVICE_TYPE == WII 
  #include "../input/WiiExtensionInput.h"
#elif DEVICE_TYPE == DIRECT
  #include "../input/DirectInput.h"
#endif
class InputHandler {
public:
  Controller controller;
  int counter;
  void processTilt();
  void init();
  void process();

#if DEVICE_TYPE == WII
  WiiExtension input;
#elif DEVICE_TYPE == DIRECT
  Direct input;
#endif
};