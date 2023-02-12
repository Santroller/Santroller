#include "shared_main.h"

#include "Arduino.h"
#include "config.h"
#include "controllers.h"
#include "endpoints.h"
#include "fxpt_math.h"
#include "io.h"
#include "pins.h"
#include "pins_define.h"
#include "ps2.h"
#include "util.h"
#include "wii.h"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12
uint8_t debounce[DIGITAL_COUNT];
uint8_t drumVelocity[8];
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
USB_Report_Data_t lastReport;
#ifdef TICK_NKRO
USB_NKRO_Data_t lastNKROReport;
#endif
#ifdef TICK_MOUSE
USB_Mouse_Data_t lastMouseReport;
#endif
#ifdef TICK_CONSUMER
USB_ConsumerControl_Data_t lastConsumerReport;
#endif
typedef struct {
    // If this bit is set, then an led effect (like star power) has overridden the leds
    uint8_t select;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Led_t;
Led_t ledState[LED_COUNT];
static const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
void init_main(void) {
    initPins();
    twi_init();
    spi_begin();
    memset(ledState, 0, sizeof(ledState));
#ifdef INPUT_PS2
    init_ack();
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
    int16_t ret = handle_calibration_xbox(orig_val, offset, min, multiplier, deadzone) >> 6;
    return (uint8_t)(ret - PS3_ACCEL_CENTER);
}

uint8_t handle_calibration_ps3_whammy(uint16_t orig_val, uint16_t min, int16_t multiplier, uint16_t deadzone) {
    int8_t ret = handle_calibration_xbox_whammy(orig_val, min, multiplier, deadzone) >> 8;
    return (uint8_t)(ret - PS3_STICK_CENTER);
}
void tick(void) {
    if (consoleType == UNIVERSAL) {
        // PS5 just stops communicating after sending a set idle
        if (set_idle && ps5_timer == 0) {
            ps5_timer = millis();
        }
        if (ps5_timer != 0 && millis() - ps5_timer > 100) {
            consoleType = PS3;
            reset_usb();
        }
        // Windows and XBOX One both send out a WCID request
        if (xbox_timer == 0 && windows_or_xbox_one) {
            xbox_timer = millis();
        }
        // But xbox follows that up with a SET_INTERFACE call, so if we don't see one of those then we can assume windows
        if (xbox_timer != 0 && millis() - xbox_timer > 1000) {
            consoleType = WINDOWS_XBOX360;
            reset_usb();
        }
        // Wii gives up after reading the config descriptor
        if (read_config && !received_after_read_config && millis() - wii_timer > 100) {
            consoleType = WII_RB;
            reset_usb();
            received_after_read_config = true;
        }
    }

    // If we have something pending to send to the xbox one controller, send it
    if (data_from_console_size) {
        send_report_to_controller(data_from_console, data_from_console_size);
        data_from_console_size = 0;
    }
}
long lastTick;
uint8_t tick_inputs(USB_Report_Data_t *combined_report) {
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
#ifdef INPUT_WT_NECK
    long pulse = WT_NECK_READ();
    if (pulse == WT_NECK_READ()) {
        lastTap = pulse;
        lastTapShift = pulse >> 1;
        lastGHWTWasSuccessful = true;
    } else {
        lastGH5WasSuccessful = false;
    }
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
    uint8_t wiiButtonsLow, wiiButtonsHigh, vel, which = 0;
    uint16_t accX, accY, accZ = 0;
    if (wiiValid) {
        memcpy(lastSuccessfulWiiPacket, wiiData, sizeof(lastSuccessfulWiiPacket));
        wiiButtonsLow = ~wiiData[4];
        wiiButtonsHigh = ~wiiData[5];
        if (hiRes) {
            wiiButtonsLow = ~wiiData[6];
            wiiButtonsHigh = ~wiiData[7];
        }
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
    TICK_SHARED;
    // Tick all three reports, and then go for the first one that has changes
    // We prioritise NKRO, then Consumer, because these are both only buttons
    // Then mouse, as it is an axis so it is more likley to have changes
#if CONSOLE_TYPE == KEYBOARD_MOUSE
    void *lastReportToCheck;
    for (int i = 1; i < REPORT_ID_END; i++) {
#ifdef TICK_MOUSE
        if (i == REPORT_ID_MOUSE) {
            size = sizeof(USB_Mouse_Data_t);
            memset(combined_report, 0, size);
            USB_Mouse_Data_t *report = (USB_Mouse_Data_t *)combined_report;
            report->rid = REPORT_ID_MOUSE;
            TICK_MOUSE;
            lastReportToCheck = &lastMouseReport;
        }
#endif
#ifdef TICK_CONSUMER
        if (i == REPORT_ID_CONSUMER) {
            size = sizeof(USB_ConsumerControl_Data_t);
            memset(combined_report, 0, size);
            USB_ConsumerControl_Data_t *report = (USB_ConsumerControl_Data_t *)combined_report;
            report->rid = REPORT_ID_CONSUMER;
            TICK_CONSUMER;
            lastReportToCheck = &lastConsumerReport;
        }
#endif
#ifdef TICK_NKRO
        if (i == REPORT_ID_NKRO) {
            size = sizeof(USB_NKRO_Data_t);
            memset(combined_report, 0, size);
            USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)combined_report;
            report->rid = REPORT_ID_NKRO;
            TICK_NKRO;
            lastReportToCheck = &lastNKROReport;
        }
#endif
        uint8_t cmp = memcmp(lastReportToCheck, combined_report, size);
        if (cmp == 0) {
            size = 0;
            continue;
        }
        memcpy(lastReportToCheck, combined_report, size);
        break;
    }
#elif CONSOLE_TYPE == MIDI

#else
    uint8_t report_size;
    bool updateSequence = false;
    bool updateHIDSequence = false;
    if (consoleType == XBOXONE) {
        if (report_sequence_number == 0) {
            report_sequence_number = 1;
        }
        if (hid_sequence_number == 0) {
            hid_sequence_number = 1;
        }
        switch (xbox_one_state) {
            case Announce:
                xbox_one_state = Waiting1;
                memcpy(combined_report, announce, sizeof(announce));
                return sizeof(announce);
            case Ident1:
                xbox_one_state = Waiting2;
                memcpy(combined_report, identify_1, sizeof(identify_1));
                return sizeof(identify_1);
            case Ident2:
                xbox_one_state = Ident3;
                memcpy(combined_report, identify_2, sizeof(identify_2));
                return sizeof(identify_2);
            case Ident3:
                xbox_one_state = Ident4;
                memcpy(combined_report, identify_3, sizeof(identify_3));
                return sizeof(identify_3);
            case Ident4:
                xbox_one_state = Waiting5;
                memcpy(combined_report, identify_4, sizeof(identify_4));
                return sizeof(identify_4);
            case Ident5:
                xbox_one_state = Auth;
                memcpy(combined_report, identify_5, sizeof(identify_5));
                return sizeof(identify_5);
            case Auth:
                if (data_from_controller_size) {
                    uint8_t size = data_from_controller_size;
                    data_from_controller_size = 0;
                    memcpy(combined_report, data_from_controller, size);
                    return size;
                }
                return 0;
            case Ready:
                break;
            default:
                return 0;
        }
    }
    if (consoleType == XBOXONE) {
        // The GHL guitar is special. It uses a standard nav report in the xbox menus, but then in game, it uses the ps3 report.
        // To switch modes, a poke command is sent every 8 seconds
        // In nav mode, we handle things like a controller, while in ps3 mode, we fall through and just set the report using ps3 mode.

        if (!DEVICE_TYPE_IS_LIVE_GUITAR || millis() - last_ghl_poke_time < 8000) {
            XBOX_ONE_REPORT *report = (XBOX_ONE_REPORT *)combined_report;
            size = sizeof(XBOX_ONE_REPORT);
            report_size = size - sizeof(GipHeader_t);
            memset(combined_report, 0, size);
            GIP_HEADER(report, GIP_INPUT_REPORT, false, report_sequence_number);
            TICK_XBOX_ONE;
            if (report->guide != lastXboxOneGuide) {
                lastXboxOneGuide = report->guide;
                GipKeystroke_t *keystroke = (GipKeystroke_t *)combined_report;
                GIP_HEADER(keystroke, GIP_VIRTUAL_KEYCODE, true, keystroke_sequence_number++);
                keystroke->pressed = report->guide;
                keystroke->keycode = GIP_VKEY_LEFT_WIN;
                return sizeof(GipKeystroke_t);
            }
            // We use an unused bit as a flag for sending the guide key code, so flip it back
            report->guide = false;
            GipPacket_t *packet = (GipPacket_t *)combined_report;
            combined_report = (USB_Report_Data_t *)packet->data;
            updateSequence = true;
        } else {
            XboxOneGHLGuitar_Data_t *report = (XboxOneGHLGuitar_Data_t *)combined_report;
            size = sizeof(XboxOneGHLGuitar_Data_t);
            report_size = sizeof(PS3_REPORT);
            memset(combined_report, 0, sizeof(XboxOneGHLGuitar_Data_t));
            GIP_HEADER(report, GHL_HID_REPORT, false, hid_sequence_number);
            combined_report = (USB_Report_Data_t *)&report->report;
            updateHIDSequence = true;
        }
    }
    if (consoleType == WINDOWS_XBOX360 || ConsoleType == STAGE_KIT) {
        XINPUT_REPORT *report = (XINPUT_REPORT *)combined_report;
        memset(combined_report, 0, sizeof(XINPUT_REPORT));
        report->rid = 0;
        report->rsize = sizeof(XINPUT_REPORT);
// Whammy on the xbox guitars goes from min to max, so it needs to default to min
#if DEVICE_TYPE_IS_GUITAR
        report->whammy = INT16_MIN;
#endif
        TICK_XINPUT;
        report_size = size = sizeof(XINPUT_REPORT);
    } else if (!updateSequence) {
        PS3_REPORT *report = (PS3_REPORT *)combined_report;
        memset(combined_report, 0, sizeof(PS3_REPORT));

        PS3Gamepad_Data_t *gamepad = (PS3Gamepad_Data_t *)combined_report;
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
        PS3Dpad_Data_t *dpad = (PS3Dpad_Data_t *)report;
        dpad->dpad = (dpad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[dpad->dpad];
        // Switch swaps a and b
        if (consoleType == SWITCH) {
            bool a = report->a;
            bool b = report->b;
            report->b = a;
            report->a = b;
        }
        report_size = size = sizeof(PS3_REPORT);
    }

    uint8_t cmp = memcmp(&lastReport, combined_report, report_size);
    if (cmp == 0) {
        return 0;
    }
    memcpy(&lastReport, combined_report, report_size);
#if CONSOLE_TYPE == UNIVERSAL || CONSOLE_TYPE == XBOXONE
    if (updateSequence) {
        report_sequence_number++;
    } else if (updateHIDSequence) {
        hid_sequence_number++;
    }
#endif
#endif
#ifdef TICK_LED
    TICK_LED;
#endif
    return size;
}

void device_reset(void) {
    xbox_one_state = Announce;
    data_from_controller_size = 0;
    data_from_console_size = 0;
    hid_sequence_number = 1;
    report_sequence_number = 1;
    last_ghl_poke_time = 0;
}

void received_any_request(void) {
    if (set_idle && ps5_timer != 0) {
        set_idle = false;
        ps5_timer = 0;
    }
    if (read_config) {
        received_after_read_config = true;
    }
}

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

void xinput_controller_connected(uint8_t vid, uint8_t pid) {
    if (xbox_360_state == Authenticated) return;
    xbox_360_vid = vid;
    xbox_360_pid = pid;
}

void xone_controller_connected(void) {
    if (xbox_one_state == Ready) return;
    GipPowerMode_t *powerMode = (GipPowerMode_t *)data_from_console;
    GIP_HEADER(powerMode, GIP_POWER_MODE_DEVICE_CONFIG, true, 1);
    send_report_to_controller(data_from_console, sizeof(GipPowerMode_t));
}
void controller_disconnected(void) {
}