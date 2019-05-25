#pragma once
#include "../../config/eeprom.h"
#include "../../io/mpu6050/inv_mpu.h"
#include "../../io/mpu6050/mpu.h"
#include "../../io/pins/Pins.h"
#include "../../util.h"
#include "../Controller.h"
#include <avr/interrupt.h>
class GuitarHandler {
public:
  bool isGuitar();
  void init();
  void handle(Controller *controller);
};