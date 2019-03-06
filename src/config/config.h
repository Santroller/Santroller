//How fast should your controller poll (note that this is expressed in 1ms frames. (depending on speed))
//Note that a 1ms poll will use a lot of bandwith, and as a result will break the ability to use
//Multiple guitars on a Hub.
#define POLL_RATE 4
#define MPU_6050 1
#define WII 1
#define DIRECT 2
#define XINPUT 1
#define KEYBOARD 2
#define TILT_SENSOR MPU_6050 // this can either be none, MPU_6050 or GRAVITY
#define DEVICE_TYPE DIRECT // this can be either WII or DIRECT
#define OUTPUT_TYPE XINPUT // this can be either XINPUT or KEYBOARD
#define DPAD 1
#define JOY 2
#if DEVICE_TYPE == DIRECT
    #define PIN_GREEN 4
    #define PIN_RED 5
    #define PIN_YELLOW 6
    #define PIN_BLUE 7
    #define PIN_ORANGE 8
    #define PIN_START 16
    #define PIN_SELECT 9
    #define PIN_LEFT 10
    #define PIN_RIGHT 21
    #define PIN_UP 14
    #define PIN_DOWN 15
    #define PIN_WHAMMY 18
    #define PIN_JOY_X 19
    #define PIN_JOY_Y 20
    #define MOVE_MODE JOY // this can either be JOY or DPAD
    #define WHAMMY_DIR -1
    #define WHAMMY_START 16863
#endif
//For a list of keyboard bindings, visit http://fourwalledcubicle.com/files/LUFA/Doc/151115/html/group___group___u_s_b_class_h_i_d_common.html
#if OUTPUT_TYPE == KEYBOARD
    #define KEY_GREEN HID_KEYBOARD_SC_A
    #define KEY_RED HID_KEYBOARD_SC_S
    #define KEY_YELLOW HID_KEYBOARD_SC_J
    #define KEY_BLUE HID_KEYBOARD_SC_K
    #define KEY_ORANGE HID_KEYBOARD_SC_L
    #define KEY_WHAMMY HID_KEYBOARD_SC_SEMICOLON_AND_COLON
    #define KEY_START HID_KEYBOARD_SC_ENTER
    #define KEY_SELECT HID_KEYBOARD_SC_H
    #define KEY_LEFT HID_KEYBOARD_SC_LEFT_ARROW
    #define KEY_RIGHT HID_KEYBOARD_SC_RIGHT_ARROW
    #define KEY_UP HID_KEYBOARD_SC_UP_ARROW
    #define KEY_DOWN HID_KEYBOARD_SC_DOWN_ARROW
#endif
#if TILT_SENSOR == GRAVITY
    #define PIN_GRAVITY 11
#endif