#include "./DirectInput.h"
void Direct::read_controller(Controller *controller) {
  auto pins = (uint8_t*)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    auto eq = i < XBOX_LB ? false : config.frets_led_mode;
    bit_write(IO::digitalRead(pins[i]) == eq, controller->buttons, i);
  }
  //TODO: add ability to copy left_stick to dpad
  //TODO: find some way to make it so that we can disable pins with a specific value
  controller->l_x = IO::analogRead(config.pins.l_x) * 32;
  controller->l_y = IO::analogRead(config.pins.l_y) * 32;
  controller->r_x = IO::analogRead(config.pins.r_x) * 32;
  controller->r_y = IO::analogRead(config.pins.r_y) * 32;
  controller->lt = IO::analogRead(config.pins.lt) * 32;
  controller->rt = IO::analogRead(config.pins.rt) * 32;
}

void Direct::init() {
  int fret_type = config.frets_led_mode ? INPUT : INPUT_PULLUP;
  auto pins = (uint8_t*)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    auto fret_type_2 = i < XBOX_LB ? INPUT_PULLUP : fret_type;
    IO::pinMode(pins[i], fret_type_2);
  }

  IO::pinMode(config.pins.l_x, INPUT);
  IO::pinMode(config.pins.l_y, INPUT);
  IO::pinMode(config.pins.r_x, INPUT);
  IO::pinMode(config.pins.r_y, INPUT);
  IO::pinMode(config.pins.lt, INPUT);
  IO::pinMode(config.pins.rt, INPUT);
}