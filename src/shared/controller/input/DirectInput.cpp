#include "./DirectInput.h"
void Direct::read_controller(Controller *controller)
{
  bit_write(IO::digitalRead(PIN_GREEN_FRET) == FRETS_LED, controller->buttons,
            GREEN);
  bit_write(IO::digitalRead(PIN_RED_FRET) == FRETS_LED, controller->buttons,
            RED);
  bit_write(IO::digitalRead(PIN_YELLOW_FRET) == FRETS_LED, controller->buttons,
            YELLOW);
  bit_write(IO::digitalRead(PIN_BLUE_FRET) == FRETS_LED, controller->buttons,
            BLUE);
  bit_write(IO::digitalRead(PIN_ORANGE_FRET) == FRETS_LED, controller->buttons,
            ORANGE);
  bit_write(!IO::digitalRead(PIN_START_BUTTON), controller->buttons, START);
  bit_write(!IO::digitalRead(PIN_SELECT_BUTTON), controller->buttons, SELECT);
  controller->r_x = (IO::analogRead(PIN_WHAMMY_POTENIOMETER) * 32) +
                    WHAMMY_INITIAL_VALUE * (INVERT_WHAMMY ? -1 : 1);
#if DIRECTION_MODE == JOY
  bit_write(!IO::digitalRead(PIN_STRUM_UP_BUTTON) ||
                IO::analogRead(PIN_JOYSTICK_Y_POTENIOMETER) > 600,
            controller->buttons, UP);
  bit_write(!IO::digitalRead(PIN_STRUM_DOWN_BUTTON) ||
                IO::analogRead(PIN_JOYSTICK_Y_POTENIOMETER) < 400,
            controller->buttons, DOWN);
  bit_write(IO::analogRead(PIN_JOYSTICK_X_POTENIOMETER) < 400,
            controller->buttons, LEFT);
  bit_write(IO::analogRead(PIN_JOYSTICK_X_POTENIOMETER) > 600,
            controller->buttons, RIGHT);
#elif DIRECTION_MODE == DPAD
  bit_write(!IO::digitalRead(PIN_STRUM_UP_BUTTON), controller->buttons, UP);
  bit_write(!IO::digitalRead(PIN_STRUM_DOWN_BUTTON), controller->buttons, DOWN);
  bit_write(!IO::digitalRead(PIN_DPAD_LEFT_BUTTON), controller->buttons, LEFT);
  bit_write(!IO::digitalRead(PIN_DPAD_RIGHT_BUTTON), controller->buttons,
            RIGHT);
#endif
}

void Direct::init()
{
  int fret_type = INPUT_PULLUP;
#if FRETS_LED == 1
  fret_type = INPUT;
#endif
  IO::pinMode(PIN_GREEN_FRET, fret_type);
  IO::pinMode(PIN_RED_FRET, fret_type);
  IO::pinMode(PIN_YELLOW_FRET, fret_type);
  IO::pinMode(PIN_BLUE_FRET, fret_type);
  IO::pinMode(PIN_ORANGE_FRET, fret_type);
  IO::pinMode(PIN_START_BUTTON, INPUT_PULLUP);
  IO::pinMode(PIN_SELECT_BUTTON, INPUT_PULLUP);
  IO::pinMode(PIN_STRUM_UP_BUTTON, INPUT_PULLUP);
  IO::pinMode(PIN_STRUM_DOWN_BUTTON, INPUT_PULLUP);
  IO::pinMode(PIN_WHAMMY_POTENIOMETER, INPUT);
#if DIRECTION_MODE == JOY
  IO::pinMode(PIN_JOYSTICK_X_POTENIOMETER, INPUT);
  IO::pinMode(PIN_JOYSTICK_Y_POTENIOMETER, INPUT);
#elif DIRECTION_MODE == DPAD
  IO::pinMode(PIN_DPAD_LEFT_BUTTON, INPUT_PULLUP);
  IO::pinMode(PIN_DPAD_RIGHT_BUTTON, INPUT_PULLUP);
#endif
}