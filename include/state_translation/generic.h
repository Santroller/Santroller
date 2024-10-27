#include "state_translation/drums.h"
#include "state_translation/guitar.h"
#include "state_translation/pro_guitar.h"
#include "state_translation/pro_keys.h"
#include "state_translation/turntable.h"
#define RAPHNET_JOYSTICK(joystick) joystick - 16000
#define PS3_JOYSTICK(joystick) (joystick >> 8) + PS3_STICK_CENTER
#define PS3_XBOX_TRIGGER(trigger) trigger >> 8
#define PS3_XBOX_TRIGGER_REV(trigger) trigger << 8
#define PS3_JOYSTICK_REV(joystick) (joystick - PS3_STICK_CENTER) << 8
#define PS3_ACCEL(accel) (accel + PS3_ACCEL_CENTER) << 6
#define PS3_ACCEL_REV(accel) (accel - PS3_ACCEL_CENTER) << 6
#define ORIGIN(joystick) joystick
// XB1 triggers range from 0 - 1024
#define XBOX_ONE_TRIGGER(trigger) trigger >> 6
#define XBOX_ONE_TRIGGER_REV(trigger) trigger << 6
#define TRANSLATE_GAMEPAD_NO_CLICK(JOYSTICK, TRIGGER, SRC, DEST) \
    DEST->a |= SRC->a;                                           \
    DEST->b |= SRC->b;                                           \
    DEST->x |= SRC->x;                                           \
    DEST->y |= SRC->y;                                           \
    DEST->leftShoulder |= SRC->leftShoulder;                     \
    DEST->rightShoulder |= SRC->rightShoulder;                   \
    if (SRC->leftTrigger) {                                      \
        DEST->leftTrigger = TRIGGER(SRC->leftTrigger);           \
    }                                                            \
    if (SRC->rightTrigger) {                                     \
        DEST->rightTrigger = TRIGGER(SRC->rightTrigger);         \
    }                                                            \
    if (SRC->leftStickX != PS3_STICK_CENTER) {                   \
        DEST->leftStickX = JOYSTICK(SRC->leftStickX);            \
    }                                                            \
    if (SRC->leftStickY != PS3_STICK_CENTER) {                   \
        DEST->leftStickY = JOYSTICK(-SRC->leftStickY);           \
    }                                                            \
    if (SRC->rightStickX != PS3_STICK_CENTER) {                  \
        DEST->rightStickX = JOYSTICK(SRC->rightStickX);          \
    }                                                            \
    if (SRC->rightStickY != PS3_STICK_CENTER) {                  \
        DEST->rightStickY = JOYSTICK(-SRC->rightStickY);         \
    }
#define TRANSLATE_GAMEPAD(JOYSTICK, TRIGGER, SRC, DEST)      \
    TRANSLATE_GAMEPAD_NO_CLICK(JOYSTICK, TRIGGER, SRC, DEST) \
    DEST->leftThumbClick |= SRC->leftThumbClick;             \
    DEST->rightThumbClick |= SRC->rightThumbClick;
#define TRANSLATE_GAMEPAD_CAPTURE(JOYSTICK, TRIGGER, SRC, DEST) \
    TRANSLATE_GAMEPAD(JOYSTICK, TRIGGER, SRC, DEST)             \
    DEST->capture |= SRC->capture;

#define TRANSLATE_GAMEPAD_PRESSURE(JOYSTICK, TRIGGER, SRC, DEST) \
    TRANSLATE_GAMEPAD(JOYSTICK, TRIGGER, SRC, DEST)              \
    if (SRC->pressureDpadUp) {                                   \
        DEST->pressureDpadUp = SRC->pressureDpadUp;              \
    }                                                            \
    if (SRC->pressureDpadRight) {                                \
        DEST->pressureDpadRight = SRC->pressureDpadRight;        \
    }                                                            \
    if (SRC->pressureDpadDown) {                                 \
        DEST->pressureDpadDown = SRC->pressureDpadDown;          \
    }                                                            \
    if (SRC->pressureDpadLeft) {                                 \
        DEST->pressureDpadLeft = SRC->pressureDpadLeft;          \
    }                                                            \
    if (SRC->pressureL1) {                                       \
        DEST->pressureL1 = SRC->pressureL1;                      \
    }                                                            \
    if (SRC->pressureR1) {                                       \
        DEST->pressureR1 = SRC->pressureR1;                      \
    }                                                            \
    if (SRC->pressureTriangle) {                                 \
        DEST->pressureTriangle = SRC->pressureTriangle;          \
    }                                                            \
    if (SRC->pressureCircle) {                                   \
        DEST->pressureCircle = SRC->pressureCircle;              \
    }                                                            \
    if (SRC->pressureCross) {                                    \
        DEST->pressureCross = SRC->pressureCross;                \
    }                                                            \
    if (SRC->pressureSquare) {                                   \
        DEST->pressureSquare = SRC->pressureSquare;              \
    }

#define DPAD_REV()                                                                 \
    uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad]; \
    asm volatile("" ::                                                             \
                     : "memory");                                                  \
    usb_host_data->dpadLeft |= dpad & LEFT;                                        \
    usb_host_data->dpadRight |= dpad & RIGHT;                                      \
    usb_host_data->dpadUp |= dpad & UP;                                            \
    usb_host_data->dpadDown |= dpad & DOWN;
