#pragma once
#include "../Controller.h"
#include "../../io/twi/I2Cdev.h"
#include "../../io/wii/NintendoExtensionCtrl.h"
#include "../../util.h"
#include <util/delay.h>

class WiiExtension {
public:
  ExtensionPort port;
  Nunchuk::Shared nchuk;
  ClassicController::Shared classic;
  DJTurntableController::Shared dj;
  GuitarController::Shared guitar;
  DrumController::Shared drum;

  void read_controller(Controller* data);
  WiiExtension();
  void init();
  short counter;
};