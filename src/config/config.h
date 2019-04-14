// How fast should your controller poll (note that this is expressed in 1ms
// frames.) Note that a 1ms poll will use a lot of bandwith, and as a result
// will break the ability to use Multiple guitars on a Hub.
#define POLL_RATE 1
#define MPU_6050 1
#define WII 1
#define DIRECT 2
#define XINPUT 1
#define KEYBOARD 2
#define TILT_SENSOR MPU_6050 // this can either be none, MPU_6050 or GRAVITY
#define DEVICE_TYPE DIRECT   // this can be either WII or DIRECT
#define OUTPUT_TYPE XINPUT   // this can be either XINPUT or KEYBOARD
#define DPAD 1
#define JOY 2
#define GUITAR_SUBTYPE 6
#define GAMEPAD_SUBTYPE 1
#define DANCE_PAD_SUBTYPE 5
#define GUITAR_ALTERNATE_SUBTYPE 7
#define DRUM_SUBTYPE 8
#define GUITAR_BASS_SUBTYPE 11
#define ARCADE_STICK_SUBTYPE 3
#define ARCADE_PAD_SUBTYPE 19
#define WHEEL_SUBTYPE 2
#define FLIGHT_STICK_SUBTYPE 4
#if DEVICE_TYPE == DIRECT
#define PIN_GREEN_FRET 4
#define PIN_RED_FRET 5
#define PIN_YELLOW_FRET 7
#define PIN_BLUE_FRET 6
#define PIN_ORANGE_FRET 8
#define PIN_START_BUTTON 16
#define PIN_SELECT_BUTTON 9
#define PIN_WHAMMY_POTENIOMETER 18
#define PIN_STRUM_UP_BUTTON 14
#define PIN_STRUM_DOWN_BUTTON 15
#define PIN_DPAD_LEFT_BUTTON 10
#define PIN_DPAD_RIGHT_BUTTON 21
#define PIN_JOYSTICK_X_POTENIOMETER 19
#define PIN_JOYSTICK_Y_POTENIOMETER 20
#define DIRECTION_MODE JOY
// Set this to JOY if your controller has a joystick, and set Joy X and Joy Y to
// the X and Y for your joystick Set this to DPAD if your controller has a DPAD,
// and set Left and Right to left and right on your dpad. Reuse pins for up and
// down and strumming, if required.
#define INVERT_WHAMMY false
// If your whammy bar appears inverted, set this to -1 to invert the output of
// your whammy bar.
#define WHAMMY_INITIAL_VALUE 16863
// Set this value to define where 0 is on your whammy bar.
#define FRETS_LED 1
// If this is set to true, the FRET pins will not use pullups, and will require
// a positive voltage to turn on. This allows for the ability to put LEDs in
// series with your frets, and accept HIGH as an input instead of LOW.
#endif
// For a list of keyboard bindings, visit
// http://fourwalledcubicle.com/files/LUFA/Doc/151115/html/group___group___u_s_b_class_h_i_d_common.html
#if OUTPUT_TYPE == KEYBOARD
#define KEY_GREEN_FRET HID_KEYBOARD_SC_A
#define KEY_RED_FRET HID_KEYBOARD_SC_S
#define KEY_YELLOW_FRET HID_KEYBOARD_SC_J
#define KEY_BLUE_FRET HID_KEYBOARD_SC_K
#define KEY_ORANGE_FRET HID_KEYBOARD_SC_L
#define KEY_WHAMMY HID_KEYBOARD_SC_SEMICOLON_AND_COLON
#define KEY_START HID_KEYBOARD_SC_ENTER
#define KEY_SELECT HID_KEYBOARD_SC_H
#define KEY_LEFT HID_KEYBOARD_SC_LEFT_ARROW
#define KEY_RIGHT HID_KEYBOARD_SC_RIGHT_ARROW
#define KEY_UP HID_KEYBOARD_SC_UP_ARROW
#define KEY_DOWN HID_KEYBOARD_SC_DOWN_ARROW
#elif OUTPUT_TYPE == XINPUT
#define XINPUT_SUBTYPE GUITAR_SUBTYPE
#endif
#if TILT_SENSOR == GRAVITY
#define PIN_GRAVITY 11
#elif TILT_SENSOR == MPU_6050
#define FLIP_MPU_6050 0
#define MPU_6050_START 28000
#endif