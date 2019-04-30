#include "./DirectInput.h"
void Direct::read_controller(Controller *controller) {
  bit_write(IO::digitalRead(config.pins.green) == config.frets_led_mode,
            controller->buttons, GREEN);
  bit_write(IO::digitalRead(config.pins.red) == config.frets_led_mode, controller->buttons,
            RED);
  bit_write(IO::digitalRead(config.pins.yellow) == config.frets_led_mode,
            controller->buttons, YELLOW);
  bit_write(IO::digitalRead(config.pins.blue) == config.frets_led_mode, controller->buttons,
            BLUE);
  bit_write(IO::digitalRead(config.pins.orange) == config.frets_led_mode,
            controller->buttons, ORANGE);
  bit_write(!IO::digitalRead(config.pins.start), controller->buttons, START);
  bit_write(!IO::digitalRead(config.pins.select), controller->buttons, SELECT);
  controller->r_x =
      (IO::analogRead(config.pins.whammy) * 32) + config.whammy_calibration;
  if (config.direction_mode == JOY) {
    bit_write(!IO::digitalRead(config.pins.strum_up) ||
                  IO::analogRead(config.pins.joy_y) > 600,
              controller->buttons, UP);
    bit_write(!IO::digitalRead(config.pins.strum_down) ||
                  IO::analogRead(config.pins.joy_y) < 400,
              controller->buttons, DOWN);
    bit_write(IO::analogRead(config.pins.joy_x) < 400, controller->buttons,
              LEFT);
    bit_write(IO::analogRead(config.pins.joy_x) > 600, controller->buttons,
              RIGHT);
  } else if (config.direction_mode == DPAD) {
    bit_write(!IO::digitalRead(config.pins.strum_up), controller->buttons, UP);
    bit_write(!IO::digitalRead(config.pins.strum_down), controller->buttons,
              DOWN);
    bit_write(!IO::digitalRead(config.pins.dpad_left), controller->buttons,
              LEFT);
    bit_write(!IO::digitalRead(config.pins.dpad_right), controller->buttons,
              RIGHT);
  }
}

void Direct::init() {
  int fret_type = config.frets_led_mode ? INPUT : INPUT_PULLUP;
  IO::pinMode(config.pins.green, fret_type);
  IO::pinMode(config.pins.red, fret_type);
  IO::pinMode(config.pins.yellow, fret_type);
  IO::pinMode(config.pins.blue, fret_type);
  IO::pinMode(config.pins.orange, fret_type);
  IO::pinMode(config.pins.start, INPUT_PULLUP);
  IO::pinMode(config.pins.select, INPUT_PULLUP);
  IO::pinMode(config.pins.strum_up, INPUT_PULLUP);
  IO::pinMode(config.pins.strum_down, INPUT_PULLUP);
  IO::pinMode(config.pins.whammy, INPUT);
  if (config.direction_mode == JOY) {
    IO::pinMode(config.pins.joy_x, INPUT);
    IO::pinMode(config.pins.joy_y, INPUT);
  } else if (config.direction_mode == DPAD) {
    IO::pinMode(config.pins.dpad_left, INPUT_PULLUP);
    IO::pinMode(config.pins.dpad_right, INPUT_PULLUP);
  }
}