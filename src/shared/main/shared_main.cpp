#include "shared_main.h"

#include "Arduino.h"
#include "bt.h"
#include "config.h"
#include "controllers.h"
#include "endpoints.h"
#include "fxpt_math.h"
#include "hid.h"
#include "io.h"
#include "io_define.h"
#include "pins.h"
#include "ps2.h"
#include "rf.h"
#include "util.h"
#include "wii.h"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12
USB_Report_Data_t combined_report;
PS3_REPORT bt_report;
uint8_t debounce[DIGITAL_COUNT];
uint8_t drumVelocity[8];
long lastSentPacket = 0;
long lastTap;
long lastTapShift;
uint16_t wiiControllerType = WII_NO_EXTENSION;
uint8_t ps2ControllerType = PSX_NO_DEVICE;
uint8_t lastSuccessfulPS2Packet[BUFFER_SIZE];
uint8_t lastSuccessfulWiiPacket[8];
uint8_t lastSuccessfulGH5Packet[2];
uint8_t lastSuccessfulTurntablePacketLeft[3];
uint8_t lastSuccessfulTurntablePacketRight[3];
long lastSuccessfulGHWTPacket;
bool lastGHWTWasSuccessful = false;
bool lastGH5WasSuccessful = false;
bool lastTurntableWasSuccessfulLeft = false;
bool lastTurntableWasSuccessfulRight = false;
bool lastWiiWasSuccessful = false;
bool lastPS2WasSuccessful = false;
bool overrideR2 = false;
bool lastXboxOneGuide = false;
uint8_t overriddenR2 = 0;
USB_LastReport_Data_t last_report_usb;
USB_LastReport_Data_t last_report_bt;
USB_LastReport_Data_t last_report_rf;
long initialWt[5] = {0};
uint8_t rawWt;
bool auth_ps4_controller_found = false;
bool seen_ps4_console = false;
#ifdef RF_TX
RfInputPacket_t rf_report = {Input};
RfHeartbeatPacket_t rf_heartbeat = {Heartbeat};
void send_rf_console_type() {
    if (rf_initialised) {
        RfConsoleTypePacket_t packet = {
            ConsoleType, consoleType};
        radio.write(&packet, sizeof(packet));
    }
}
#endif
#ifdef RF
#include "SPI.h"
#include "rf.h"
RF24 radio(RADIO_CE, RADIO_CSN);
uint8_t rf_data[32];
#endif
#ifdef RF_RX
uint64_t addresses[RF_COUNT] = RF_ADDRESSES;
RfInputPacket_t last_rf_inputs[RF_COUNT];
#endif
typedef struct {
    // If this bit is set, then an led effect (like star power) has overridden the leds
    uint8_t select;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Led_t;
Led_t ledState[LED_COUNT];
#define UP 1 << 0
#define DOWN 1 << 1
#define LEFT 1 << 2
#define RIGHT 1 << 3
static const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
static const uint8_t dpad_bindings_reverse[] = {UP, UP | RIGHT, RIGHT, DOWN | RIGHT, DOWN, DOWN | LEFT, LEFT, UP | LEFT};

#ifdef INPUT_WT_NECK
uint8_t gh5_mapping[] = {
    0x00,
    0x95,
    0xCD,
    0xB0,
    0x1A,
    0x19,
    0xE6,
    0xE5,
    0x49,
    0x47,
    0x48,
    0x46,
    0x2F,
    0x2D,
    0x2E,
    0x2C,
    0x7F,
    0x7B,
    0x7D,
    0x79,
    0x7E,
    0x7A,
    0x7C,
    0x78,
    0x66,
    0x62,
    0x64,
    0x60,
    0x65,
    0x61,
    0x63,
    0x5F,
};
bool checkWt(int pin) {
    return readWt(pin) > initialWt[pin];
}
uint8_t readWtAnalog() {
    return gh5_mapping[rawWt];
}
#endif
void init_main(void) {
    initPins();
    twi_init();
    spi_begin();
    memset(ledState, 0, sizeof(ledState));
#ifdef INPUT_PS2
    init_ack();
#endif
#ifdef RADIO_MOSI
    SPI.setTX(RADIO_MOSI);
    SPI.setRX(RADIO_MISO);
    SPI.setSCK(RADIO_SCK);
#endif
#ifdef RF
    rf_initialised = radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.enableDynamicPayloads();
    radio.enableAckPayload();
#endif
#ifdef RF_TX
    radio.openWritingPipe((uint64_t)TRANSMIT_RADIO_ID);  // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, (uint64_t)DEST_RADIO_ID);
    radio.stopListening();
    send_rf_console_type();
#endif
#ifdef RF_RX
    radio.openWritingPipe((uint64_t)DEST_RADIO_ID);  // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    for (int i = 0; i < RF_COUNT; i++) {
        radio.openReadingPipe(i + 1, addresses[i]);
    }
    radio.startListening();
#endif
#ifdef INPUT_WT_NECK
    memset(initialWt, 0, sizeof(initialWt));
    for (int j = 0; j < 50; j++) {
        for (int i = 0; i < 5; i++) {
            long reading = readWt(i) + WT_SENSITIVITY;
            if (reading > initialWt[i]) {
                initialWt[i] = reading;
            }
        }
    }
#endif
}
int16_t adc_i(uint8_t pin) {
    int32_t ret = adc(pin);
    return ret - 32767;
}
int16_t handle_calibration_xbox(int16_t orig_val, int16_t offset, int16_t min, int16_t multiplier, int16_t deadzone) {
    int32_t val = orig_val;
    int16_t val_deadzone = orig_val - offset;
    if (val_deadzone < deadzone && val_deadzone > -deadzone) {
        return 0;
    }
    if (val < 0) {
        deadzone = -deadzone;
    }
    val -= deadzone;
    val -= min;
    val *= multiplier;
    val /= 512;
    val += INT16_MIN;
    if (val > INT16_MAX) {
        val = INT16_MAX;
    }
    if (val < INT16_MIN) {
        val = INT16_MIN;
    }
    return val;
}

int16_t handle_calibration_xbox_whammy(uint16_t orig_val, uint16_t min, int16_t multiplier, uint16_t deadzone) {
    int32_t val = orig_val;
    if (multiplier > 0) {
        if ((val - min) < deadzone) {
            return INT16_MIN;
        }
    } else {
        if (val > min) {
            return INT16_MIN;
        }
    }
    val -= min;
    val *= multiplier;
    val /= 512;
    val -= INT16_MAX;
    if (val > INT16_MAX) {
        val = INT16_MAX;
    }
    if (val < INT16_MIN) {
        val = INT16_MIN;
    }
    return val;
}
uint16_t handle_calibration_xbox_one_trigger(uint16_t orig_val, uint16_t min, int16_t multiplier, uint16_t deadzone) {
    int32_t val = orig_val;
    if (multiplier > 0) {
        if ((val - min) < deadzone) {
            return 0;
        }
    } else {
        if (val > min) {
            return 0;
        }
    }
    val -= min;
    val *= multiplier;
    val /= 512;
    if (val > UINT16_MAX) {
        val = UINT16_MAX;
    }
    if (val < 0) {
        val = 0;
    }
    return val;
}
uint8_t handle_calibration_ps3(int16_t orig_val, int16_t offset, int16_t min, int16_t multiplier, int16_t deadzone) {
    int8_t ret = handle_calibration_xbox(orig_val, offset, min, multiplier, deadzone) >> 8;
    return (uint8_t)(ret - PS3_STICK_CENTER);
}

uint8_t handle_calibration_ps3_360_trigger(uint16_t orig_val, uint16_t min, int16_t multiplier, uint16_t deadzone) {
    return handle_calibration_xbox_one_trigger(orig_val, min, multiplier, deadzone) >> 8;
}

uint16_t handle_calibration_ps3_accel(uint16_t orig_val, int16_t offset, uint16_t min, int16_t multiplier, uint16_t deadzone) {
#if DEVICE_TYPE == GUITAR
    // For whatever reason, the acceleration for guitars swings between -128 to 128, not -512 to 512
    int16_t ret = (handle_calibration_xbox(orig_val, offset, min, multiplier, deadzone) >> 8) - GUITAR_ONE_G;
#else
    int16_t ret = handle_calibration_xbox(orig_val, offset, min, multiplier, deadzone) >> 6;
#endif
    return PS3_ACCEL_CENTER + ret;
}

uint8_t handle_calibration_ps3_whammy(uint16_t orig_val, uint16_t min, int16_t multiplier, uint16_t deadzone) {
    int8_t ret = handle_calibration_xbox_whammy(orig_val, min, multiplier, deadzone) >> 8;
    return (uint8_t)(ret - PS3_STICK_CENTER);
}

uint8_t tick_xbox_one() {
    switch (xbox_one_state) {
        case Announce:
            xbox_one_state = Waiting1;
            memcpy(&combined_report, announce, sizeof(announce));
            return sizeof(announce);
        case Ident1:
            xbox_one_state = Waiting2;
            memcpy(&combined_report, identify_1, sizeof(identify_1));
            return sizeof(identify_1);
        case Ident2:
            xbox_one_state = Ident3;
            memcpy(&combined_report, identify_2, sizeof(identify_2));
            return sizeof(identify_2);
        case Ident3:
            xbox_one_state = Ident4;
            memcpy(&combined_report, identify_3, sizeof(identify_3));
            return sizeof(identify_3);
        case Ident4:
            xbox_one_state = Waiting5;
            memcpy(&combined_report, identify_4, sizeof(identify_4));
            return sizeof(identify_4);
        case Ident5:
            xbox_one_state = Auth;
            memcpy(&combined_report, identify_5, sizeof(identify_5));
            return sizeof(identify_5);
        case Auth:
            if (data_from_controller_size) {
                uint8_t size = data_from_controller_size;
                data_from_controller_size = 0;
                memcpy(&combined_report, data_from_controller, size);
                return size;
            }
            return 0;
        case Ready:
            return 0;
        default:
            return 0;
    }
}

long lastTick;
uint8_t keyboard_report = 0;
// TODO: PS3 RB top frets (maybe we additionally stuff both sets of frets into one of the pressure bytes, and then zero it for non RF or BT)
// TODO: same for RB drums and cymbal velocity
#if defined(RF_RX) || BLUETOOTH
// When we do RF and Bluetooth, the reports are ALWAYS in PS3 Instrument format, so we need to convert
void convert_ps3_to_type(uint8_t *buf, PS3_REPORT *report, uint8_t output_console_type) {
    uint8_t dpad = report->dpad > RIGHT ? 0 : dpad_bindings_reverse[report->dpad];
    bool up = dpad & UP;
    bool down = dpad & DOWN;
    bool left = dpad & LEFT;
    bool right = dpad & RIGHT;
    bool universal_report = output_console_type == UNIVERSAL || output_console_type == REAL_PS3;
#if DEVICE_TYPE_IS_INSTRUMENT
    universal_report |= output_console_type == PS3;
#elif DEVICE_TYPE_IS_GAMEPAD
    if (consoleType == PS3) {
        PS3Gamepad_Data_t *out = (PS3Gamepad_Data_t *)buf;
        if (report->leftStickX != PS3_STICK_CENTER) {
            out->leftStickX = report->leftStickX;
        }
        if (report->leftStickY != PS3_STICK_CENTER) {
            out->leftStickY = report->leftStickY;
        }
        if (report->rightStickX != PS3_STICK_CENTER) {
            out->rightStickX = report->rightStickX;
        }
        if (report->rightStickY != PS3_STICK_CENTER) {
            out->rightStickY = report->rightStickY;
        }
        if (report->leftTrigger) {
            out->leftTrigger = report->leftTrigger;
        }
        if (report->rightTrigger) {
            out->rightTrigger = report->rightTrigger;
        }
        if (report->pressureDpadUp) {
            out->pressureDpadUp = report->pressureDpadUp;
        }
        if (report->pressureDpadRight) {
            out->pressureDpadRight = report->pressureDpadRight;
        }
        if (report->pressureDpadDown) {
            out->pressureDpadDown = report->pressureDpadDown;
        }
        if (report->pressureDpadLeft) {
            out->pressureDpadLeft = report->pressureDpadLeft;
        }
        if (report->pressureL1) {
            out->pressureL1 = report->pressureL1;
        }
        if (report->pressureR1) {
            out->pressureR1 = report->pressureR1;
        }
        if (report->pressureTriangle) {
            out->pressureTriangle = report->pressureTriangle;
        }
        if (report->pressureCircle) {
            out->pressureCircle = report->pressureCircle;
        }
        if (report->pressureCross) {
            out->pressureCross = report->pressureCross;
        }
        if (report->pressureSquare) {
            out->pressureSquare = report->pressureSquare;
        }
        out->dpadUp |= up;
        out->dpadDown |= down;
        out->dpadLeft |= left;
        out->dpadRight |= right;
        out->x |= report->x;
        out->a |= report->a;
        out->b |= report->b;
        out->y |= report->y;

        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
        out->l2 |= report->l2;
        out->r2 |= report->r2;

        out->back |= report->back;
        out->start |= report->start;
        out->leftThumbClick |= report->leftThumbClick;
        out->rightThumbClick |= report->rightThumbClick;

        out->guide |= report->guide;
    }
#endif
    if (consoleType == PS4) {
        PS4_REPORT *out = (PS4_REPORT *)buf;
        out->x |= report->x;
        out->a |= report->a;
        out->b |= report->b;
        out->y |= report->y;

        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
        out->l2 |= report->l2;
        out->r2 |= report->r2;

        out->back |= report->back;
        out->start |= report->start;
        out->leftThumbClick |= report->leftThumbClick;
        out->rightThumbClick |= report->rightThumbClick;

        out->guide |= report->guide;
        out->capture |= report->capture;
        if (report->dpad) {
            out->dpad = report->dpad;
        }
#if DEVICE_TYPE_IS_LIVE_GUITAR
        if (report->tilt_pc != PS3_STICK_CENTER) {
            out->tilt = report->tilt_pc;
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = report->whammy;
        }
        if (report->strumBar != PS3_STICK_CENTER) {
            out->strumBar = report->strumBar;
        }
#elif DEVICE_TYPE_IS_GAMEPAD
        if (report->leftStickX != PS3_STICK_CENTER) {
            out->leftStickX = report->leftStickX;
        }
        if (report->leftStickY != PS3_STICK_CENTER) {
            out->leftStickY = report->leftStickY;
        }
        if (report->rightStickX != PS3_STICK_CENTER) {
            out->rightStickX = report->rightStickX;
        }
        if (report->rightStickY != PS3_STICK_CENTER) {
            out->rightStickY = report->rightStickY;
        }
        if (report->leftTrigger) {
            out->leftTrigger = report->leftTrigger;
        }
        if (report->rightTrigger) {
            out->rightTrigger = report->rightTrigger;
        }
#endif
    }
    if (consoleType == XBOXONE) {
        XBOX_ONE_REPORT *out = (XBOX_ONE_REPORT *)buf;
        out->x |= report->x;
        out->a |= report->a;
        out->b |= report->b;
        out->y |= report->y;

        out->dpadUp |= up;
        out->dpadDown |= down;
        out->dpadLeft |= left;
        out->dpadRight |= right;
        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;

        out->back |= report->back;
        out->start |= report->start;
        out->leftThumbClick |= report->leftThumbClick;
        out->rightThumbClick |= report->rightThumbClick;

        out->guide |= report->guide;
#if DEVICE_TYPE_IS_GAMEPAD
        if (report->leftStickX != PS3_STICK_CENTER) {
            out->leftStickX = (report->leftStickX - 128) << 8;
        }
        if (report->leftStickY != PS3_STICK_CENTER) {
            out->leftStickY = (report->leftStickY - 128) << 8;
        }
        if (report->rightStickX != PS3_STICK_CENTER) {
            out->rightStickX = (report->rightStickX - 128) << 8;
        }
        if (report->rightStickY != PS3_STICK_CENTER) {
            out->rightStickY = (report->rightStickY - 128) << 8;
        }
        if (report->leftTrigger) {
            out->leftTrigger = (report->leftTrigger) << 8;
        }
        if (report->rightTrigger) {
            out->rightTrigger = (report->rightTrigger) << 8;
        }
#elif DEVICE_TYPE_IS_LIVE_GUITAR
        if (report->tilt_pc != PS3_STICK_CENTER) {
            out->tilt = report->tilt_pc;
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = report->whammy;
        }
        if (report->strumBar != PS3_STICK_CENTER) {
            out->strumBar = report->strumBar;
        }
#elif DEVICE_TYPE_IS_GUITAR
        if (report->tilt_pc != PS3_STICK_CENTER) {
            out->tilt = report->tilt_pc;
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = report->whammy;
        }
        if (report->pickup != PS3_STICK_CENTER) {
            out->pickup = report->pickup;
        }
#elif DEVICE_TYPE_IS_DRUM
        if (report->yellowVelocity) {
            out->yellowVelocity = report->yellowVelocity >> 4;
        }
        if (report->redVelocity) {
            out->redVelocity = report->redVelocity >> 4;
        }
        if (report->greenVelocity) {
            out->greenVelocity = report->greenVelocity >> 4;
        }
        if (report->blueVelocity) {
            out->blueVelocity = report->blueVelocity >> 4;
        }
#endif
    }
    if (consoleType == WINDOWS_XBOX360) {
        XINPUT_REPORT *out = (XINPUT_REPORT *)buf;
        out->x |= report->x;
        out->a |= report->a;
        out->b |= report->b;
        out->y |= report->y;
        out->dpadUp |= up;
        out->dpadDown |= down;
        out->dpadLeft |= left;
        out->dpadRight |= right;

        out->back |= report->back;
        out->start |= report->start;

        out->guide |= report->guide;
#if DEVICE_TYPE_IS_GAMEPAD
        if (report->leftStickX != PS3_STICK_CENTER) {
            out->leftStickX = (report->leftStickX - 128) << 8;
        }
        if (report->leftStickY != PS3_STICK_CENTER) {
            out->leftStickY = (report->leftStickY - 128) << 8;
        }
        if (report->rightStickX != PS3_STICK_CENTER) {
            out->rightStickX = (report->rightStickX - 128) << 8;
        }
        if (report->rightStickY != PS3_STICK_CENTER) {
            out->rightStickY = (report->rightStickY - 128) << 8;
        }
        if (report->leftTrigger) {
            out->leftTrigger = report->leftTrigger;
        }
        if (report->rightTrigger) {
            out->rightTrigger = report->rightTrigger;
        }

        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
        out->leftThumbClick |= report->leftThumbClick;
        out->rightThumbClick |= report->rightThumbClick;
#elif DEVICE_TYPE == DRUMS && RHYTHM_TYPE == GUITAR_HERO
        if (report->yellowVelocity) {
            out->yellowVelocity = report->yellowVelocity;
        }
        if (report->redVelocity) {
            out->redVelocity = report->redVelocity;
        }
        if (report->greenVelocity) {
            out->greenVelocity = report->greenVelocity;
        }
        if (report->blueVelocity) {
            out->blueVelocity = report->blueVelocity;
        }
        if (report->kickVelocity) {
            out->kickVelocity = report->kickVelocity;
        }
        if (report->orangeVelocity) {
            out->orangeVelocity = report->orangeVelocity;
        }
#elif DEVICE_TYPE == DRUMS && RHYTHM_TYPE == ROCK_BAND
        if (report->yellowVelocity) {
            out->yellowVelocity = -(0x7FFF - (report->yellowVelocity << 7));
        }
        if (report->redVelocity) {
            out->redVelocity = (0x7FFF - (report->redVelocity << 7));
        }
        if (report->greenVelocity) {
            out->greenVelocity = -(0x7FFF - (report->greenVelocity << 7));
        }
        if (report->blueVelocity) {
            out->blueVelocity = (0x7FFF - (report->blueVelocity << 7));
        }
        out->padFlag = report->padFlag;
        out->cymbalFlag = report->cymbalFlag;
#elif DEVICE_TYPE_IS_LIVE_GUITAR
        if (report->tilt_pc != PS3_STICK_CENTER) {
            out->tilt = (report->tilt_pc - 128) << 8;
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = (report->whammy - 128) << 8;
        }
        if (report->strumBar != PS3_STICK_CENTER) {
            out->strumBar = (report->strumBar - 128) << 8;
        }
#elif DEVICE_TYPE == GUITAR && RHYTHM_TYPE == GUITAR_HERO
        if (report->tilt_pc != PS3_STICK_CENTER) {
            out->tilt = (report->tilt_pc - 128) << 8;
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = (report->whammy - 128) << 8;
        }
        if (report->slider != PS3_STICK_CENTER) {
            out->slider = (report->slider) << 8;
        }
#elif DEVICE_TYPE == GUITAR && RHYTHM_TYPE == ROCK_BAND
        if (report->tilt_pc != PS3_STICK_CENTER) {
            out->tilt = (report->tilt_pc - 128) << 8;
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = (report->whammy - 128) << 8;
        }
        if (report->pickup != PS3_STICK_CENTER) {
            out->pickup = report->pickup;
        }
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE

        if (report->leftTableVelocity != PS3_STICK_CENTER) {
            out->leftTableVelocity = (report->leftTableVelocity - 128) << 8;
        }
        if (report->rightTableVelocity != PS3_STICK_CENTER) {
            out->rightTableVelocity = (report->rightTableVelocity - 128) << 8;
        }
        if (report->effectsKnob != PS3_ACCEL_CENTER) {
            out->effectsKnob = (report->effectsKnob - 128) << 8;
        }
        if (report->crossfader != PS3_ACCEL_CENTER) {
            out->crossfader = (report->crossfader - 128) << 8;
        }
        out->leftBlue |= report->leftBlue;
        out->leftRed |= report->leftRed;
        out->leftGreen |= report->leftGreen;
        out->rightBlue |= report->rightBlue;
        out->rightRed |= report->rightRed;
        out->rightGreen |= report->rightGreen;
#endif
    }
    if (universal_report) {
        PS3_REPORT *out = (PS3_REPORT *)buf;
        out->x |= report->x;
        out->a |= report->a;
        out->b |= report->b;
        out->y |= report->y;

        out->back |= report->back;
        out->start |= report->start;

        out->guide = report->guide;
        out->capture = report->capture;
        if (report->dpad) {
            out->dpad = report->dpad;
        }
#if DEVICE_TYPE_IS_GAMEPAD
        if (report->leftStickX != PS3_STICK_CENTER) {
            out->leftStickX = report->leftStickX;
        }
        if (report->leftStickY != PS3_STICK_CENTER) {
            out->leftStickY = report->leftStickY;
        }
        if (report->rightStickX != PS3_STICK_CENTER) {
            out->rightStickX = report->rightStickX;
        }
        if (report->rightStickY != PS3_STICK_CENTER) {
            out->rightStickY = report->rightStickY;
        }
        if (report->leftTrigger) {
            out->leftTrigger = report->leftTrigger;
        }
        if (report->rightTrigger) {
            out->rightTrigger = report->rightTrigger;
        }
        if (report->pressureDpadUp) {
            out->pressureDpadUp = report->pressureDpadUp;
        }
        if (report->pressureDpadRight) {
            out->pressureDpadRight = report->pressureDpadRight;
        }
        if (report->pressureDpadDown) {
            out->pressureDpadDown = report->pressureDpadDown;
        }
        if (report->pressureDpadLeft) {
            out->pressureDpadLeft = report->pressureDpadLeft;
        }
        if (report->pressureL1) {
            out->pressureL1 = report->pressureL1;
        }
        if (report->pressureR1) {
            out->pressureR1 = report->pressureR1;
        }
        if (report->pressureTriangle) {
            out->pressureTriangle = report->pressureTriangle;
        }
        if (report->pressureCircle) {
            out->pressureCircle = report->pressureCircle;
        }
        if (report->pressureCross) {
            out->pressureCross = report->pressureCross;
        }
        if (report->pressureSquare) {
            out->pressureSquare = report->pressureSquare;
        }
        out->l2 |= report->l2;
        out->r2 |= report->r2;
        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
        out->leftThumbClick |= report->leftThumbClick;
        out->rightThumbClick |= report->rightThumbClick;
#elif DEVICE_TYPE == DRUMS && RHYTHM_TYPE == GUITAR_HERO
        if (report->yellowVelocity) {
            out->yellowVelocity = report->yellowVelocity;
        }
        if (report->redVelocity) {
            out->redVelocity = report->redVelocity;
        }
        if (report->greenVelocity) {
            out->greenVelocity = report->greenVelocity;
        }
        if (report->blueVelocity) {
            out->blueVelocity = report->blueVelocity;
        }
        if (report->kickVelocity) {
            out->kickVelocity = report->kickVelocity;
        }
        if (report->orangeVelocity) {
            out->orangeVelocity = report->orangeVelocity;
        }
        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
#elif DEVICE_TYPE == DRUMS && RHYTHM_TYPE == ROCK_BAND
        if (report->yellowVelocity) {
            out->yellowVelocity = report->yellowVelocity;
        }
        if (report->redVelocity) {
            out->redVelocity = report->redVelocity;
        }
        if (report->greenVelocity) {
            out->greenVelocity = report->greenVelocity;
        }
        if (report->blueVelocity) {
            out->blueVelocity = report->blueVelocity;
        }
        out->padFlag = report->padFlag;
        out->cymbalFlag = report->cymbalFlag;
        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
#elif DEVICE_TYPE_IS_LIVE_GUITAR
        if (report->tilt_pc != PS3_STICK_CENTER) {
            if (output_console_type == PS3 || output_console_type == REAL_PS3) {
                out->tilt = (PS3_ACCEL_CENTER + (report->tilt_pc - 128) - GUITAR_ONE_G);
            } else {
                out->tilt_pc = report->tilt_pc;
            }
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = report->whammy;
        }
        if (report->strumBar != PS3_STICK_CENTER) {
            out->strumBar = report->strumBar;
        }
        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
#elif DEVICE_TYPE == GUITAR && RHYTHM_TYPE == GUITAR_HERO
        if (report->tilt_pc != PS3_STICK_CENTER) {
            if (output_console_type == PS3 || output_console_type == REAL_PS3) {
                out->tilt = (PS3_ACCEL_CENTER + (report->tilt_pc - 128) - GUITAR_ONE_G);
            } else {
                out->tilt_pc = report->tilt_pc;
            }
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = report->whammy;
        }
        if (report->slider != PS3_STICK_CENTER) {
            out->slider = report->slider;
        }
        out->leftShoulder |= report->leftShoulder;
        out->rightShoulder |= report->rightShoulder;
#elif DEVICE_TYPE == GUITAR && RHYTHM_TYPE == ROCK_BAND
        if (report->tilt_pc != PS3_STICK_CENTER) {
            // TODO: what are we actually setting tilt_pc to these days?
            if (output_console_type == PS3 || output_console_type == REAL_PS3) {
                out->tilt = report->tilt_pc > 128;
            } else {
                out->tilt_pc = report->tilt_pc;
            }
        }
        if (report->whammy != PS3_STICK_CENTER) {
            out->whammy = report->whammy;
        }
        if (report->pickup != PS3_STICK_CENTER) {
            out->pickup = report->pickup;
        }
        out->leftShoulder |= report->leftShoulder;
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE

        if (report->leftTableVelocity != PS3_STICK_CENTER) {
            out->leftTableVelocity = report->leftTableVelocity;
        }
        if (report->rightTableVelocity != PS3_STICK_CENTER) {
            out->rightTableVelocity = report->rightTableVelocity;
        }
        if (report->effectsKnob != PS3_ACCEL_CENTER) {
            out->effectsKnob = report->effectsKnob;
        }
        if (report->crossfader != PS3_ACCEL_CENTER) {
            out->crossfader = report->crossfader;
        }
        out->leftBlue |= report->leftBlue;
        out->leftRed |= report->leftRed;
        out->leftGreen |= report->leftGreen;
        out->rightBlue |= report->rightBlue;
        out->rightRed |= report->rightRed;
        out->rightGreen |= report->rightGreen;
        out->tableNeutral |= report->tableNeutral;
#endif
    }
}
#endif
#ifndef RF_RX
uint8_t tick_inputs(void *buf, USB_LastReport_Data_t *last_report, uint8_t output_console_type) {
    uint8_t size = 0;
    // Tick Inputs
#ifdef INPUT_DJ_TURNTABLE
    uint8_t *dj_left = lastSuccessfulTurntablePacketLeft;
    uint8_t *dj_right = lastSuccessfulTurntablePacketRight;
    bool djLeftValid = twi_readFromPointer(DJ_TWI_PORT, DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketLeft), dj_left);
    bool djRightValid = twi_readFromPointer(DJ_TWI_PORT, DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketRight), dj_right);
    lastTurntableWasSuccessfulLeft = djLeftValid;
    lastTurntableWasSuccessfulRight = djRightValid;
#endif
#ifdef INPUT_GH5_NECK
    uint8_t *fivetar_buttons = lastSuccessfulGH5Packet;
    bool gh5Valid = twi_readFromPointer(GH5_TWI_PORT, GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(lastSuccessfulGH5Packet), lastSuccessfulGH5Packet);
    lastGH5WasSuccessful = gh5Valid;
#endif
#ifdef INPUT_PS2
    uint8_t *ps2Data = tickPS2();
    bool ps2Valid = ps2Data != NULL;
    lastPS2WasSuccessful = ps2Valid;
    if (ps2Valid) {
        memcpy(lastSuccessfulPS2Packet, ps2Data, sizeof(lastSuccessfulPS2Packet));
    }
#endif
#ifdef INPUT_WII
    uint8_t *wiiData;
    // If we didn't send the last packet, then we need to wait some time as the wii controllers do not like being polled quickly
    if (micros() - lastTick > 800) {
        lastTick = micros();
        wiiData = tickWii();
    } else {
        wiiData = lastSuccessfulWiiPacket;
    }
    bool wiiValid = wiiData != NULL;
    lastWiiWasSuccessful = wiiValid;
    uint8_t wiiButtonsLow, wiiButtonsHigh, vel, which, lastTapWiiGh5, lastTapWii = 0;
    uint16_t accX, accY, accZ = 0;
    if (wiiValid) {
        memcpy(lastSuccessfulWiiPacket, wiiData, sizeof(lastSuccessfulWiiPacket));
        wiiButtonsLow = ~wiiData[4];
        wiiButtonsHigh = ~wiiData[5];
        if (hiRes) {
            wiiButtonsLow = ~wiiData[6];
            wiiButtonsHigh = ~wiiData[7];
        }
#ifdef INPUT_WII_TAP
        lastTapWii = (wiiData[2] & 0x1f);
        // GH3 guitars set this bit, while WT and GH5 guitars do not
        if (wiiData[0] & (1 << 7) == 0) {
            lastTapWii = 0;
        }
        if (lastTapWii == 0x0f) {
            lastTapWiiGh5 = 0;
        } else if (lastTapWii == 0x04) {
            lastTapWiiGh5 = 0x95;
        } else if (lastTapWii == 0x07) {
            lastTapWiiGh5 = 0xB0;
        } else if (lastTapWii == 0x0A) {
            lastTapWiiGh5 = 0xCD;
        } else if (lastTapWii == 0x0C || lastTapWii == 0x0D) {
            lastTapWiiGh5 = 0xE6;
        } else if (lastTapWii == 0x12 || lastTapWii == 0x13) {
            lastTapWiiGh5 = 0x1A;
        } else if (lastTapWii == 0x14 || lastTapWii == 0x15) {
            lastTapWiiGh5 = 0x2F;
        } else if (lastTapWii == 0x17 || lastTapWii == 0x18) {
            lastTapWiiGh5 = 0x49;
        } else if (lastTapWii == 0x1A) {
            lastTapWiiGh5 = 0x66;
        } else if (lastTapWii == 0x1F) {
            lastTapWiiGh5 = 0x7F;
        }

#endif
#ifdef INPUT_WII_DRUM
        vel = (7 - (wiiData[3] >> 5)) << 5;
        which = (wiiData[2] & 0b01111100) >> 1;
        switch (which) {
            case 0x1B:
                drumVelocity[DRUM_KICK] = vel;
                break;
            case 0x12:
                drumVelocity[DRUM_GREEN] = vel;
                break;
            case 0x19:
                drumVelocity[DRUM_RED] = vel;
                break;
            case 0x11:
                drumVelocity[DRUM_YELLOW] = vel;
                break;
            case 0x0F:
                drumVelocity[DRUM_BLUE] = vel;
                break;
            case 0x0E:
                drumVelocity[DRUM_ORANGE] = vel;
                break;
        }
#endif
#ifdef INPUT_WII_NUNCHUK
        accX = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
        accY = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
        accZ = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
#endif
    }
#endif
#ifdef INPUT_WT_NECK
    rawWt = checkWt(4) | (checkWt(3) << 1) | (checkWt(2) << 2) | (checkWt(0) << 3) | (checkWt(1) << 4);
#endif
    TICK_SHARED;
    // We tick the guitar every 5ms to handle inputs if nothing is attempting to read, but this doesn't need to output that data anywhere.
    if (!buf) return 0;
    // Handle button combos for detection logic
    if (millis() < 2000 && consoleType == UNIVERSAL) {
        TICK_DETECTION;
    }
    // Tick all three reports, and then go for the first one that has changes
    // We prioritise NKRO, then Consumer, because these are both only buttons
    // Then mouse, as it is an axis so it is more likley to have changes
#if CONSOLE_TYPE == KEYBOARD_MOUSE
    void *lastReportToCheck;
    for (int i = 1; i < REPORT_ID_END; i++) {
#ifdef TICK_MOUSE
        if (i == REPORT_ID_MOUSE) {
            size = sizeof(USB_Mouse_Data_t);
            memset(buf, 0, size);
            USB_Mouse_Data_t *report = (USB_Mouse_Data_t *)buf;
            report->rid = REPORT_ID_MOUSE;
            TICK_MOUSE;
            if (last_report) {
                lastReportToCheck = &last_report->lastMouseReport;
            }
        }
#endif
#ifdef TICK_CONSUMER
        if (i == REPORT_ID_CONSUMER) {
            size = sizeof(USB_ConsumerControl_Data_t);
            memset(buf, 0, size);
            USB_ConsumerControl_Data_t *report = (USB_ConsumerControl_Data_t *)buf;
            report->rid = REPORT_ID_CONSUMER;
            TICK_CONSUMER;
            if (last_report) {
                lastReportToCheck = &last_report->lastConsumerReport;
            }
        }
#endif
#ifdef TICK_NKRO
        if (i == REPORT_ID_NKRO) {
            size = sizeof(USB_NKRO_Data_t);
            memset(buf, 0, size);
            USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)buf;
            report->rid = REPORT_ID_NKRO;
            TICK_NKRO;
            if (last_report) {
                lastReportToCheck = &last_report->lastNKROReport;
            }
        }
#endif
        // If we are directly asked for a HID report, always just reply with the NKRO one
        if (lastReportToCheck) {
            uint8_t cmp = memcmp(lastReportToCheck, buf, size);
            if (cmp == 0) {
                size = 0;
                continue;
            }
            memcpy(lastReportToCheck, buf, size);
            break;
        } else {
            break;
        }
    }
#else
    bool rf_or_bluetooth = buf == &last_bt_report || buf == &last_report_rf.lastControllerReport;
    USB_Report_Data_t *report_data = (USB_Report_Data_t *)buf;
    uint8_t report_size;
    bool updateSequence = false;
    bool updateHIDSequence = false;
    if (output_console_type == XBOXONE) {
        // The GHL guitar is special. It uses a standard nav report in the xbox menus, but then in game, it uses the ps3 report.
        // To switch modes, a poke command is sent every 8 seconds
        // In nav mode, we handle things like a controller, while in ps3 mode, we fall through and just set the report using ps3 mode.

        if (!DEVICE_TYPE_IS_LIVE_GUITAR || millis() - last_ghl_poke_time < 8000) {
            XBOX_ONE_REPORT *report = (XBOX_ONE_REPORT *)buf;
            size = sizeof(XBOX_ONE_REPORT);
            report_size = size - sizeof(GipHeader_t);
            memset(buf, 0, size);
            GIP_HEADER(report, GIP_INPUT_REPORT, false, report_sequence_number);
            TICK_XBOX_ONE;
            if (report->guide != lastXboxOneGuide) {
                lastXboxOneGuide = report->guide;
                GipKeystroke_t *keystroke = (GipKeystroke_t *)buf;
                GIP_HEADER(keystroke, GIP_VIRTUAL_KEYCODE, true, keystroke_sequence_number++);
                keystroke->pressed = report->guide;
                keystroke->keycode = GIP_VKEY_LEFT_WIN;
                return sizeof(GipKeystroke_t);
            }
            // We use an unused bit as a flag for sending the guide key code, so flip it back
            report->guide = false;
            GipPacket_t *packet = (GipPacket_t *)buf;
            report_data = (USB_Report_Data_t *)packet->data;
            updateSequence = true;
        } else {
            XboxOneGHLGuitar_Data_t *report = (XboxOneGHLGuitar_Data_t *)buf;
            size = sizeof(XboxOneGHLGuitar_Data_t);
            report_size = sizeof(PS3_REPORT);
            memset(buf, 0, sizeof(XboxOneGHLGuitar_Data_t));
            GIP_HEADER(report, GHL_HID_REPORT, false, hid_sequence_number);
            report_data = (USB_Report_Data_t *)&report->report;
            updateHIDSequence = true;
        }
    }
    if (output_console_type == WINDOWS_XBOX360 || output_console_type == STAGE_KIT) {
        XINPUT_REPORT *report = (XINPUT_REPORT *)report_data;
        memset(report_data, 0, sizeof(XINPUT_REPORT));
        report->rid = 0;
        report->rsize = sizeof(XINPUT_REPORT);
// Whammy on the xbox guitars goes from min to max, so it needs to default to min
#if DEVICE_TYPE_IS_GUITAR
        report->whammy = INT16_MIN;
#endif
        TICK_XINPUT;
        report_size = size = sizeof(XINPUT_REPORT);
    }
// Guitars and Drums can fall back to their PS3 versions, so don't even include the PS4 version there.
// DJ Hero was never on ps4, so we can't really implement that either, so just fall back to PS3 there too.
#if SUPPORTS_PS4
    if (output_console_type == PS4) {
        if (millis() > 450000 && !auth_ps4_controller_found) {
            reset_usb();
        }
        PS4_REPORT *report = (PS4_REPORT *)report_data;
        memset(report, 0, sizeof(PS4_REPORT));
        PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)report;
        gamepad->report_id = 0x01;
        gamepad->leftStickX = PS3_STICK_CENTER;
        gamepad->leftStickY = PS3_STICK_CENTER;
        gamepad->rightStickX = PS3_STICK_CENTER;
        gamepad->rightStickY = PS3_STICK_CENTER;
        // PS4 does not start using the controller until it sees a PS button press.
        if (!seen_ps4_console) {
            report->guide = true;
        }
        TICK_PS4;
        gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
        report_size = size = sizeof(PS4_REPORT);
    }
#endif
// If we are dealing with a non instrument controller (like a gamepad) then we use the proper ps3 controller report format, to allow for emulator support and things like that
// This also gives us PS2 support via PADEMU and wii support via fakemote for standard controllers.
// However, actual ps3 support was being a pain so we use the instrument descriptor there, since the ps3 doesn't care.
#if !(DEVICE_TYPE_IS_INSTRUMENT)
    if (output_console_type == PS3) {
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)report_data;
        memset(report, 0, sizeof(PS3_REPORT));
        report->reportId = 1;
        report->accelX = PS3_ACCEL_CENTER;
        report->accelY = PS3_ACCEL_CENTER;
        report->accelZ = PS3_ACCEL_CENTER;
        report->gyro = PS3_ACCEL_CENTER;
        report->leftStickX = PS3_STICK_CENTER;
        report->leftStickY = PS3_STICK_CENTER;
        report->rightStickX = PS3_STICK_CENTER;
        report->rightStickY = PS3_STICK_CENTER;
        TICK_PS3;
        report_size = size = sizeof(PS3Gamepad_Data_t);
    }
    if (output_console_type != WINDOWS_XBOX360 && output_console_type != PS3 && output_console_type != PS4 && output_console_type != STAGE_KIT && !updateHIDSequence) {
#else
    // For instruments, we instead use the below block, as our universal and PS3 descriptors use the same report format in that case
    if (output_console_type != WINDOWS_XBOX360 && output_console_type != PS4 && output_console_type != STAGE_KIT && !updateHIDSequence) {
#endif
        PS3_REPORT *report = (PS3_REPORT *)report_data;
        memset(report, 0, sizeof(PS3_REPORT));
        PCGamepad_Data_t *gamepad = (PCGamepad_Data_t *)report;
        gamepad->accelX = PS3_ACCEL_CENTER;
        gamepad->accelY = PS3_ACCEL_CENTER;
        gamepad->accelZ = PS3_ACCEL_CENTER;
        gamepad->gyro = PS3_ACCEL_CENTER;
        gamepad->leftStickX = PS3_STICK_CENTER;
        gamepad->leftStickY = PS3_STICK_CENTER;
        gamepad->rightStickX = PS3_STICK_CENTER;
        gamepad->rightStickY = PS3_STICK_CENTER;
        TICK_PS3;
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
        if (!report->leftBlue && !report->leftRed && !report->leftGreen && !report->rightBlue && !report->rightRed && !report->rightGreen) {
            report->tableNeutral = true;
        }
#endif
        gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
        // Switch swaps a and b
        if (output_console_type == SWITCH) {
            bool a = report->a;
            bool b = report->b;
            report->b = a;
            report->a = b;
        }
        report_size = size = sizeof(PS3_REPORT);
    }
    // If we are being asked for a HID report (aka via HID_GET_REPORT), then just send whatever inputs we have, do not compare
    // PS4 controllers always update
    if (consoleType != PS4 && last_report) {
        uint8_t cmp = memcmp(&last_report->lastControllerReport, report_data, report_size);
        if (cmp == 0) {
            return 0;
        }
        memcpy(&last_report->lastControllerReport, report_data, report_size);
    }
// Standard PS4 controllers need a report counter, but we don't want to include that when ticking
#if DEVICE_TYPE_IS_GAMEPAD
    if (consoleType == PS4) {
        PS4Gamepad_Data_t *gamepad = (PS4Gamepad_Data_t *)report_data;
        gamepad->reportCounter = ps4_sequence_number++;
    }
#endif
#if CONSOLE_TYPE == UNIVERSAL || CONSOLE_TYPE == XBOXONE
    if (updateSequence) {
        report_sequence_number++;
        if (report_sequence_number == 0) {
            report_sequence_number = 1;
        }
    } else if (updateHIDSequence) {
        hid_sequence_number++;
        if (hid_sequence_number == 0) {
            hid_sequence_number = 1;
        }
    }
#endif
#endif
    return size;
}
#else
uint8_t tick_inputs(void *buf, USB_LastReport_Data_t *last_report, uint8_t output_console_type) {
    uint8_t rf_size;
    uint8_t size;
    uint8_t pipe;
    bool updated = false;
    if (radio.available(&pipe)) {
        rf_connected = true;
        radio.read(rf_data, sizeof(rf_data));
        rf_size = radio.getDynamicPayloadSize();
        switch (rf_data[0]) {
            case ConsoleType:
                set_console_type(rf_data[1]);
                if (last_report) {
                    return 0;
                }
                break;
            case Input:
                updated = true;
                break;
            case Heartbeat:
                if (last_report) {
                    return 0;
                }
                break;
        }
#if DEVICE_TYPE_KEYBOARD
        if (!updated) {
            size = sizeof(USB_NKRO_Data_t);
            memcpy(&last_report->lastNKROReport, buf, size);
            return size;
        }
#ifdef TICK_NKRO
        if (rf_data[1] == REPORT_ID_NKRO) {
            memcpy(&last_rf_inputs[pipe].lastNKROReport, rf_data + 1, rf_size - 1);
        }
#endif
#ifdef TICK_MOUSE
        if (rf_data[1] == REPORT_ID_MOUSE) {
            memcpy(&last_rf_inputs[pipe].lastMouseReport, rf_data + 1, rf_size - 1);
        }
#endif
#ifdef TICK_CONSUMER
        if (rf_data[1] == REPORT_ID_CONSUMER) {
            memcpy(&last_rf_inputs[pipe].lastConsumerReport, rf_data + 1, rf_size - 1);
        }
#endif
        void *lastReportToCheck;
        for (int i = 1; i < REPORT_ID_END; i++) {
            for (int dev = 0; dev < RF_COUNT; dev++) {
#ifdef TICK_MOUSE
                if (i == REPORT_ID_MOUSE) {
                    USB_Mouse_Data_t *report = (USB_Mouse_Data_t *)buf;
                    size = sizeof(USB_Mouse_Data_t);
                    if (dev == 0) {
                        memset(buf, 0, size);
                    }
                    report->rid = REPORT_ID_MOUSE;
                    // Since the mouse report has analog inputs we need to handle it manually
                    USB_Mouse_Data_t *report_from_rf = &last_rf_inputs[pipe].lastMouseReport;
                    report->left |= report_from_rf->left;
                    report->right |= report_from_rf->right;
                    report->middle |= report_from_rf->middle;
                    if (report_from_rf->X) {
                        report->X = report_from_rf->X;
                    }
                    if (report_from_rf->Y) {
                        report->Y = report_from_rf->Y;
                    }
                    if (report_from_rf->ScrollX) {
                        report->ScrollX = report_from_rf->ScrollX;
                    }
                    if (report_from_rf->ScrollY) {
                        report->ScrollY = report_from_rf->ScrollY;
                    }
                    if (last_report) {
                        lastReportToCheck = &last_report->lastMouseReport;
                    }
                }
#endif
#ifdef TICK_CONSUMER
                if (i == REPORT_ID_CONSUMER) {
                    size = sizeof(USB_ConsumerControl_Data_t);
                    if (dev == 0) {
                        memset(buf, 0, size);
                    }
                    // Consumer is easy, as we are only dealing with bits per button, so ORing is fine
                    uint8_t *current_report = (uint8_t *)&last_rf_inputs[pipe];
                    for (size_t j = 0; j < size; j++) {
                        buf[j] |= current_report[j];
                    }
                    if (last_report) {
                        lastReportToCheck = &last_report->lastConsumerReport;
                    }
                }
#endif
#ifdef TICK_NKRO
                if (i == REPORT_ID_NKRO) {
                    size = sizeof(USB_NKRO_Data_t);
                    if (dev == 0) {
                        memset(buf, 0, size);
                    }
                    // NKRO is easy, as we are only dealing with bits per button, so ORing is fine
                    uint8_t *current_report = (uint8_t *)&last_rf_inputs[pipe];
                    for (size_t j = 0; j < size; j++) {
                        buf[j] |= current_report[j];
                    }
                    if (last_report) {
                        lastReportToCheck = &last_report->lastNKROReport;
                    }
                }
#endif
            }
            // If we are directly asked for a HID report, always just reply with the NKRO one
            if (lastReportToCheck) {
                uint8_t cmp = memcmp(lastReportToCheck, buf, size);
                if (cmp == 0) {
                    size = 0;
                    continue;
                }
                memcpy(lastReportToCheck, buf, size);
                break;
            } else {
                break;
            }
        }
#else
        memcpy(&last_rf_inputs[pipe].lastControllerReport, rf_data + 1, rf_size - 1);
        USB_Report_Data_t *report_data = (USB_Report_Data_t *)buf;
        uint8_t report_size;
        bool updateSequence = false;
        bool updateHIDSequence = false;
        if (consoleType == XBOXONE) {
            // The GHL guitar is special. It uses a standard nav report in the xbox menus, but then in game, it uses the ps3 report.
            // To switch modes, a poke command is sent every 8 seconds
            // In nav mode, we handle things like a controller, while in ps3 mode, we fall through and just set the report using ps3 mode.

            if (!DEVICE_TYPE_IS_LIVE_GUITAR || millis() - last_ghl_poke_time < 8000) {
                XBOX_ONE_REPORT *report = (XBOX_ONE_REPORT *)buf;
                size = sizeof(XBOX_ONE_REPORT);
                report_size = size - sizeof(GipHeader_t);
                if (updated) {
                    memset(buf, 0, size);
                }
                GIP_HEADER(report, GIP_INPUT_REPORT, false, report_sequence_number);
                if (updated) {
                    for (int dev = 0; dev < RF_COUNT; dev++) {
                        convert_ps3_to_type(buf, &last_rf_inputs[dev].lastControllerReport, consoleType);
                    }
                }
                if (report->guide != lastXboxOneGuide) {
                    lastXboxOneGuide = report->guide;
                    GipKeystroke_t *keystroke = (GipKeystroke_t *)buf;
                    GIP_HEADER(keystroke, GIP_VIRTUAL_KEYCODE, true, keystroke_sequence_number++);
                    keystroke->pressed = report->guide;
                    keystroke->keycode = GIP_VKEY_LEFT_WIN;
                    return sizeof(GipKeystroke_t);
                }
                // We use an unused bit as a flag for sending the guide key code, so flip it back
                report->guide = false;
                GipPacket_t *packet = (GipPacket_t *)buf;
                report_data = (USB_Report_Data_t *)packet->data;
                updateSequence = true;
            } else {
                XboxOneGHLGuitar_Data_t *report = (XboxOneGHLGuitar_Data_t *)buf;
                size = sizeof(XboxOneGHLGuitar_Data_t);
                report_size = sizeof(PS3_REPORT);
                if (updated) {
                    memset(buf, 0, sizeof(XboxOneGHLGuitar_Data_t));
                }
                GIP_HEADER(report, GHL_HID_REPORT, false, hid_sequence_number);
                report_data = (USB_Report_Data_t *)&report->report;
                updateHIDSequence = true;
            }
        }
        if (consoleType == WINDOWS_XBOX360 || consoleType == STAGE_KIT) {
            XINPUT_REPORT *report = (XINPUT_REPORT *)report_data;
            if (updated) {
                memset(report_data, 0, sizeof(XINPUT_REPORT));
                report->rid = 0;
                report->rsize = sizeof(XINPUT_REPORT);
// Whammy on the xbox guitars goes from min to max, so it needs to default to min
#if DEVICE_TYPE_IS_GUITAR
                report->whammy = INT16_MIN;
#endif
                for (int dev = 0; dev < RF_COUNT; dev++) {
                    convert_ps3_to_type((uint8_t *)report_data, &last_rf_inputs[dev].lastControllerReport, WINDOWS_XBOX360);
                }
            }
            report_size = size = sizeof(XINPUT_REPORT);
        }
// Guitars and Drums can fall back to their PS3 versions, so don't even include the PS4 version there.
// DJ Hero was never on ps4, so we can't really implement that either, so just fall back to PS3 there too.
#if SUPPORTS_PS4
        if (consoleType == PS4) {
            PS4_REPORT *report = (PS4_REPORT *)report_data;
            if (updated) {
                PS4Gamepad_Data_t *gamepad = (PS4Gamepad_Data_t *)report;
                gamepad->report_id = 0x01;
                gamepad->leftStickX = PS3_STICK_CENTER;
                gamepad->leftStickY = PS3_STICK_CENTER;
                gamepad->rightStickX = PS3_STICK_CENTER;
                gamepad->rightStickY = PS3_STICK_CENTER;
#if !DEVICE_TYPE_IS_LIVE_GUITAR
                gamepad->reportCounter = ps4_sequence_number;
#endif

                for (int dev = 0; dev < RF_COUNT; dev++) {
                    convert_ps3_to_type((uint8_t *)report_data, &last_rf_inputs[dev].lastControllerReport, consoleType);
                }
            }
            report_size = size = sizeof(PS4_REPORT);
        }
#endif
// If we are dealing with a non instrument controller (like a gamepad) then we use the proper ps3 controller report format, to allow for emulator support and things like that
// This also gives us PS2 support via PADEMU and wii support via fakemote for standard controllers.
// However, actual ps3 support was being a pain so we use the instrument descriptor there, since the ps3 doesn't care.
#if !(DEVICE_TYPE_IS_INSTRUMENT)
        if (consoleType == PS3) {
            PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)report_data;

            if (updated) {
                memset(report, 0, sizeof(PS3_REPORT));
                report->reportId = 1;
                for (int dev = 0; dev < RF_COUNT; dev++) {
                    convert_ps3_to_type((uint8_t *)report_data, &last_rf_inputs[dev].lastControllerReport, consoleType);
                }
                report_size = size = sizeof(PS3Gamepad_Data_t);
            }
            if (consoleType != WINDOWS_XBOX360 && consoleType != PS3 && consoleType != PS4 && consoleType != STAGE_KIT && !updateHIDSequence) {
#else
        // For instruments, we instead use the below block, as our universal and PS3 descriptors use the same report format in that case
        if (consoleType != WINDOWS_XBOX360 && consoleType != PS4 && consoleType != STAGE_KIT && !updateHIDSequence) {
#endif
                PS3_REPORT *report = (PS3_REPORT *)report_data;
                memset(report, 0, sizeof(PS3_REPORT));
                PCGamepad_Data_t *gamepad = (PCGamepad_Data_t *)report;
                gamepad->accelX = PS3_ACCEL_CENTER;
                gamepad->accelY = PS3_ACCEL_CENTER;
                gamepad->accelZ = PS3_ACCEL_CENTER;
                gamepad->gyro = PS3_ACCEL_CENTER;
                gamepad->leftStickX = PS3_STICK_CENTER;
                gamepad->leftStickY = PS3_STICK_CENTER;
                gamepad->rightStickX = PS3_STICK_CENTER;
                gamepad->rightStickY = PS3_STICK_CENTER;
                for (int dev = 0; dev < RF_COUNT; dev++) {
                    convert_ps3_to_type((uint8_t *)report_data, &last_rf_inputs[dev].lastControllerReport, REAL_PS3);
                }
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
                if (!report->leftBlue && !report->leftRed && !report->leftGreen && !report->rightBlue && !report->rightRed && !report->rightGreen) {
                    report->tableNeutral = true;
                }
#endif
                // Switch swaps a and b
                if (consoleType == SWITCH) {
                    bool a = report->a;
                    bool b = report->b;
                    report->b = a;
                    report->a = b;
                }
            }
            report_size = size = sizeof(PS3_REPORT);
        }
        // If we are being asked for a HID report (aka via HID_GET_REPORT), then just send whatever inputs we have, do not compare
        if (last_report) {
            uint8_t cmp = memcmp(&last_report->lastControllerReport, report_data, report_size);
            if (cmp == 0) {
                return 0;
            }
            memcpy(&last_report->lastControllerReport, report_data, report_size);
        }
        if (consoleType == PS4) {
            ps4_sequence_number++;
        }
#if CONSOLE_TYPE == UNIVERSAL || CONSOLE_TYPE == XBOXONE
        if (updateSequence) {
            report_sequence_number++;
            if (report_sequence_number == 0) {
                report_sequence_number = 1;
            }
        } else if (updateHIDSequence) {
            hid_sequence_number++;
            if (hid_sequence_number == 0) {
                hid_sequence_number = 1;
            }
        }
#endif
#endif
    }
    return size;
}
#endif

#ifdef RF_TX
void tick_rf_tx(void) {
    uint8_t size = 0;
    uint8_t pipe;
    if (radio.available(&pipe)) {
        rf_connected = true;
        radio.read(rf_data, sizeof(rf_data));
        switch (rf_data[0]) {
            case AckAuthLed:
                handle_auth_led();
                break;
            case AckPlayerLed:
                handle_player_leds(rf_data[1]);
                break;
            case AckRumble:
                handle_rumble(rf_data[1], rf_data[2]);
                break;
            case AckKeyboardLed:
                handle_keyboard_leds(rf_data[1]);
                break;
        }
    }
    size = tick_inputs(&rf_report.lastControllerReport, &last_report_rf, UNIVERSAL);
    if (size > 0) {
        rf_connected = radio.write(&rf_report, size + 1);
    } else {
        rf_connected = radio.write(&rf_heartbeat, sizeof(rf_heartbeat));
    }
    return;
}
#endif
#if BLUETOOTH
bool tick_bluetooth(void) {
    uint8_t size = tick_inputs&bt_report, &last_report_bt, UNIVERSAL);
    if (size) {
        send_report(size, (uint8_t *)&bt_report);
    }
    return size;
}
#endif
#ifndef RF_ONLY
bool tick_usb(void) {
    // If we have something pending to send to the xbox one controller, send it
    uint8_t size = 0;
    bool ready = ready_for_next_packet();
    if (!ready) return 0;
    if (data_from_console_size) {
        send_report_to_controller(XBOXONE, data_from_console, data_from_console_size);
        data_from_console_size = 0;
    }
    if (consoleType == XBOXONE && xbox_one_state != Ready) {
        size = tick_xbox_one();
    }
    if (!size) {
        size = tick_inputs(&combined_report, &last_report_usb, consoleType);
    }
    send_report_to_pc(&combined_report, size);
    seen_ps4_console = true;
    return size;
}
#endif
void tick(void) {
#ifdef TICK_LED
    TICK_LED;
#endif
#ifndef RF_ONLY
    bool ready = tick_usb();
#endif
#ifdef RF_TX
    tick_rf_tx();
#endif
#if BLUETOOTH
    tick_bluetooth();
#endif
#if !defined(RF_TX) && !BLUETOOTH
    // Tick the controller every 5ms if this device is usb only, and usb is not ready
    if (!ready && millis() - lastSentPacket > 5) {
        lastSentPacket = millis();
        tick_inputs(NULL, NULL, consoleType);
    }
#endif
}

void device_reset(void) {
    xbox_one_state = Announce;
    data_from_controller_size = 0;
    data_from_console_size = 0;
    hid_sequence_number = 1;
    report_sequence_number = 1;
    last_ghl_poke_time = 0;
}

uint8_t last_len = false;
void receive_report_from_controller(uint8_t const *report, uint16_t len) {
    if (xbox_one_state != Auth) {
        return;
    }
    data_from_controller_size = len;
    memcpy(data_from_controller, report, len);
    if (report[0] == GIP_INPUT_REPORT) {
        report_sequence_number = report[2] + 1;
    }
}

void xinput_controller_connected(uint8_t vid, uint8_t pid, uint8_t subtype) {
    if (subtype == XINPUT_STAGE_KIT) {
        passthrough_stage_kit = true;
    }
    if (xbox_360_state == Authenticated) return;
    xbox_360_vid = vid;
    xbox_360_pid = pid;
}

void xone_controller_connected(void) {
    if (xbox_one_state == Ready) return;

    GipPowerMode_t *powerMode = (GipPowerMode_t *)data_from_console;
    GIP_HEADER(powerMode, GIP_POWER_MODE_DEVICE_CONFIG, true, 1);
    powerMode->subcommand = 0x00;
    send_report_to_controller(XBOXONE, data_from_console, sizeof(GipPowerMode_t));
}

void ps4_controller_connected(void) {
    ps4_output_report report = {
        report_id : 0x05,
        valid_flag0 : 0xFF,
        lightbar_red : 0x00,
        lightbar_green : 0x00,
        lightbar_blue : 0xFF
    };
    send_report_to_controller(PS4, (uint8_t *)&report, sizeof(report));
    auth_ps4_controller_found = true;
}

void controller_disconnected(void) {
    auth_ps4_controller_found = false;
}

void set_console_type(uint8_t new_console_type) {
    if (consoleType == new_console_type) return;
    consoleType = new_console_type;
    reset_usb();
}