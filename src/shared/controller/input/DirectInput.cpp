#include "./DirectInput.h"
void Direct::read_controller(Controller *controller) {
  auto pins = (uint8_t *)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    auto eq = i < XBOX_LB ? false : config.frets_led_mode;
    bit_write(IO::digitalRead(pins[i]) == eq, controller->buttons, i);
  }
  READ_JOY(l_x);
  READ_JOY(l_y);
  READ_JOY(r_x);
  READ_JOY(r_y);
  READ_JOY(lt);
  READ_JOY(rt);
}

void Direct::init() {
  int fret_type = config.frets_led_mode ? INPUT : INPUT_PULLUP;
  auto pins = (uint8_t *)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    if (pins[i] != INVALID_PIN) {
      auto fret_type_2 = i < XBOX_LB ? INPUT_PULLUP : fret_type;
      IO::pinMode(pins[i], fret_type_2);
    }
  }

  DEFINE_JOY(l_x);
  DEFINE_JOY(l_y);
  DEFINE_JOY(r_x);
  DEFINE_JOY(r_y);
  DEFINE_JOY(lt);
  DEFINE_JOY(rt);
}