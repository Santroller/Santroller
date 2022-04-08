#include "lib_main.h"

#include <stddef.h>
#include <string.h>

#include "defines.h"
#include "i2c.h"
#include "leds.h"
#include "spi_impl.h"
#include "usb/controller_reports.h"
#define PIN_COUNT PORTS* PINS_PER_PORT
#ifdef __AVR__
#define SPI_FREQ F_CPU / 2
#else
#define SPI_FREQ 8000000
#endif
bool read_hid_report_descriptor = false;
ConsoleType_t consoleType;
DeviceType_t deviceType;
InputType_t inputType;
TiltType_t tiltType;
FretLedMode_t ledMode;
PCMode_t pcMode;
bool guitar;
bool drum;
uint8_t pinCount = 0;
Input_t pins[PIN_COUNT] = {0};
AnalogInput_t analogInfo[NUM_ANALOG_INPUTS] = {0};
uint32_t lastMillisStrum;
uint8_t inputCount;
bool mergedStrum = false;
void (*tickInput)(void) = NULL;
unsigned long start;
// Inside leds there is a colour and a forceColor
// if forceColor is set or the button is pressed, then the colour is set to color
// otherwise, it is cleared
Input_t* ledOrder[PIN_COUNT] = {0};
const PROGMEM uint8_t xinputToPs3[16] = {
    PS3_DPAD_UP_BT, PS3_DPAD_DOWN_BT, PS3_DPAD_LEFT_BT, PS3_DPAD_RIGHT_BT,
    PS3_START_BT, PS3_SELECT_BT, PS3_LEFT_STICK_BT, PS3_RIGHT_STICK_BT,
    PS3_L1_BT, PS3_R1_BT, PS3_PS_BT, INVALID_PIN,
    PS3_CROSS_BT, PS3_CIRCLE_BT, PS3_SQUARE_BT, PS3_TRIANGLE_BT};
uint8_t strumUp = XBOX_DPAD_UP;
uint8_t strumDown = XBOX_DPAD_DOWN;
void init() {
    ledMode = NONE;
    inputType = DIRECT;
    consoleType = PC;
    deviceType = GUITAR_HERO_GUITAR;
    // TODO: this will differ for PS3 or keyboard
    inputCount = XBOX_AXIS_COUNT + XBOX_BTN_COUNT;
    guitar = deviceType == GUITAR_HERO_GUITAR || deviceType == ROCK_BAND_GUITAR || deviceType == GUITAR_HERO_LIVE_GUITAR;
    drum = deviceType == GUITAR_HERO_DRUMS || deviceType == ROCK_BAND_DRUMS;
    tiltType = NONE;
    if (tiltType == MPU_6050 || inputType == WII) {
        twi_init();
    }
    if (inputType != PS2 && ledMode == APA102) {
        spi_begin(SPI_FREQ, true, true, false);
    }
    initPins();
    if (inputType == WII) {
        // Init wii, load in the wii inputs as the "pins" instead
        // One interesting thing is we should be able to just load in tilt as a standard pin here too.
    } else if (inputType == PS2) {
        // Init ps2, load in the ps2 inputs as the "pins" instead
    }
    if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH || consoleType == PC) {
        for (int i = 0; i < inputCount; i++) {
            Input_t* input = pins + i;
            input->binding = xinputToPs3[input->binding];
        }
        strumUp = PS3_DPAD_UP;
        strumDown = PS3_DPAD_DOWN;
    } else if (consoleType == KEYBOARD_MOUSE) {
        strumUp = THID_KEYBOARD_SC_UP_ARROW;
        strumDown = THID_KEYBOARD_SC_DOWN_ARROW;
    }
    start = millis();
}

uint32_t last = 0;
uint8_t lastHat = 0;
bool keyboardReport = true;
uint8_t firstKeyboardPin = 0;
bool test = false;
uint8_t tick(uint8_t* data) {
    if (tickInput) {
        tickInput();
    }
    uint8_t* triggers;
    int16_t* joys = NULL;
    uint8_t* joys8 = NULL;
    uint8_t* buttons;
    uint8_t* hats = NULL;
    size_t size;
    int start = 0;
    int end = pinCount;
    if (consoleType == KEYBOARD_MOUSE) {
        // If firstKeyboardPin is 0, than the user has not specified any mouse bindings
        // So we can skip the mouse
        if (keyboardReport || !firstKeyboardPin) {
            start = firstKeyboardPin;
            TUSB_KeyboardReport_Data_t* report = (TUSB_KeyboardReport_Data_t*)data;
            report->rid = REPORT_ID_KBD;
            buttons = report->KeyCodeFlags;
            size = sizeof(TUSB_KeyboardReport_Data_t);
        } else {
            start = 0;
            TUSB_MouseReport_Data_t* report = (TUSB_MouseReport_Data_t*)data;
            report->rid = REPORT_ID_MOUSE;
            buttons = &report->Button;
            joys8 = &report->X;
            size = sizeof(TUSB_MouseReport_Data_t);
            end = firstKeyboardPin;
        }
        keyboardReport = !keyboardReport;
    } else if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
        USB_XInputReport_Arr_Data_t* report = (USB_XInputReport_Arr_Data_t*)data;
        report->rid = 0;
        report->rsize = sizeof(USB_XInputReport_Arr_Data_t);
        size = sizeof(USB_XInputReport_Data_t);
        triggers = report->triggers;
        joys = report->joys;
        buttons = (uint8_t*)&report->buttons;
    } else if (consoleType == MIDI) {
        // This one will need something specific as it isnt a controller.
    } else {
        USB_PS3Report_Arr_Data_t* report = (USB_PS3Report_Arr_Data_t*)data;
        size = sizeof(USB_PS3Report_Arr_Data_t);
        joys8 = report->joys;
        triggers = report->axis + PS3_L2;
        buttons = (uint8_t*)&report->buttons;
        hats = &report->hat;
        report->hat = lastHat;
        // How do we want to handle analogue pressures (axis?)
    }
    for (int i = start; i < end; i++) {
        Input_t* pin = pins + i;
        if (pin->axisInfo && !pin->axisInfo->isADC) {
            int16_t val = pin->axisInfo->analogRead(pin);
            uint8_t val2 = (val >> 8) + 127;
            if (pin->binding <= XBOX_RT) {
                triggers[pin->binding] = val2;
            } else if (joys) {
                joys[pin->binding - XBOX_RT] = val;
            } else {
                joys8[pin->binding] = val2;
            }
        } else {
            bool val = pin->digitalRead(pin);
            uint32_t current = millis();
            uint32_t last = pin->lastMillis;
            bool usingMerged = mergedStrum && (pin->binding == strumUp || pin->binding == strumDown);
            if (usingMerged) {
                last = lastMillisStrum;
            }
            if (current - last > pin->milliDeBounce) {
                // >>3 is equiv to /8 (2^3), %8 is equiv to & 7 (8-1)
                bit_write(val, buttons[pin->binding >> 3], pin->binding & 7);
                pin->lastMillis = current;
                if (usingMerged) {
                    lastMillisStrum = current;
                }
            }
        }
    }
    if (hats) {
        // To simlpify things, the above code treats the hat as if it is buttons.
        // Now, we need to transform that into an actual hat
        static uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07,
                                         0x05, 0x08, 0x02, 0x01, 0x03};
        // Note that due to debounce, we need to remember the last had value before it was transformed
        // As we need to be able to keep it active when the hats are being debounced.
        lastHat = hats[0];
        hats[0] = lastHat > 0x0a ? 0x08 : hat_bindings[lastHat];
    }
    if (ledMode == APA102) {
        tickLEDs(ledOrder);
    }
    // bit_write(test, buttons[PS3_SELECT_BT >> 3], PS3_SELECT_BT & 7);
    // bit_write(test, buttons[PS3_START_BT >> 3], PS3_START_BT & 7);
    // bit_write(test, buttons[XBOX_A << 3], XBOX_A & 7);
    test = !test;
    if (!read_hid_report_descriptor && millis() - start > 1000 && consoleType == PC) {
        consoleType = WII_RB;
        reset_usb();
    }
    return size;
}
void packetReceived(uint8_t* data, uint8_t len) {
    if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
        if (data[0] == 0x01) {
            XInputLEDReport_t* leds = (XInputLEDReport_t*)data;
            (void)leds;
            // We can receive led reports here and do things with them
        } else if (data[0] == 0x00) {
            XInputRumbleReport_t* rumble = (XInputRumbleReport_t*)data;
            (void)rumble;
            // We can receive rumble reports here and do things with them
        }
    } else if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH || consoleType == PC) {
        ps3_output_report* report = (ps3_output_report*)data;
        (void)report;
    }
}