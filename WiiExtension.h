#ifndef _WiiExtension_H_
#define _WiiExtension_H_
#include <NintendoExtensionCtrl.h>
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
  ExtensionController extension;
  Nunchuk::Data nchuk = Nunchuk::Data(extension);
  ClassicController::Data classic = ClassicController::Data(extension);
  DrumController::Data drum = DrumController::Data(extension);
  GuitarController::Data guitar = GuitarController::Data(extension);
  DJTurntableController::Data dj = DJTurntableController::Data(extension);
  void read_controller(WiiController* controller);
  void setup();
};

#endif
