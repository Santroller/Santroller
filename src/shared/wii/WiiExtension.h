#pragma once
#include "../Controller.h"
#include "../twi/I2Cdev.h"
#include "util/delay.h"
#include "../mpu6050/mpu.h"
#include "../mpu6050/inv_mpu.h"
#include "NintendoExtensionCtrl.h"

class WiiExtension {
public:
  ExtensionPort port;
  Nunchuk::Shared nchuk;
  ClassicController::Shared classic;
  DJTurntableController::Shared dj;
  GuitarController::Shared guitar;
  DrumController::Shared drum;

  void read_controller(USB_JoystickReport_Data_t* data);
  WiiExtension();
  void init();
  short counter;
};