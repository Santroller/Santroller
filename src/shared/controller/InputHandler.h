#include "Controller.h"
#include "../../config/config.h"
#include "../mpu6050/inv_mpu.h"
#include "../mpu6050/mpu.h"
#include "../direct/IO.h"
#if DEVICE_TYPE == WII 
  #include "../wii/WiiExtension.h"
#elif DEVICE_TYPE == DIRECT || TILT_SENSOR == GRAVITY
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