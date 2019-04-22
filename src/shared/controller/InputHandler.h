#include "Controller.h"
#include "../../config/config.h"
#include "../direct/IO.h"
#if DEVICE_TYPE == WII 
  #include "../wii/WiiExtension.h"
#elif DEVICE_TYPE == DIRECT
  #include "../direct/Direct.h"
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
#elif DEVICE_TYPE == DIRECT || TILT_SENSOR == GRAVITY
  Direct input;
#endif
};