#pragma once
#include <NintendoExtensionCtrl.h>

// digital_buttons_1
#define XBOX_DPAD_UP    0x01
#define XBOX_DPAD_DOWN    0x02
#define XBOX_DPAD_LEFT    0x04
#define XBOX_DPAD_RIGHT   0x08
#define XBOX_START      0x10
#define XBOX_BACK     0x20
#define XBOX_LEFT_STICK   0x40
#define XBOX_RIGHT_STICK  0x80

// digital_buttons_2
#define XBOX_LB   0x01
#define XBOX_RB   0x02
#define XBOX_HOME 0x04
#define XBOX_A    0x10
#define XBOX_B    0x20
#define XBOX_X    0x40
#define XBOX_Y    0x80

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
  bool valid = false;
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
