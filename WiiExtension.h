#ifndef _WiiExtension_H_
#define _WiiExtension_H_
#include <NintendoExtensionCtrl.h>
#include "Wire.h"
#define INTERRUPT_PIN 7
#define LED_PIN 13

typedef struct {
	uint8_t rid;
	uint8_t rsize;
  uint8_t digital_buttons_1;
  uint8_t digital_buttons_2;
  uint8_t lt;
  uint8_t rt;
  int l_x;
  int l_y;
  int r_x;
  int r_y;
	uint8_t reserved_1[6];
} WiiController;
class WiiExtension {
  public:
  ExtensionPort port;
  Nunchuk::Shared nchuk;
  ClassicController::Shared classic;
  DrumController::Shared drum;
  GuitarController::Shared guitar;
  DJTurntableController::Shared dj;
  boolean read_controller(WiiController* controller, float* ypr);
  void setup();
  WiiExtension();
};

#endif
