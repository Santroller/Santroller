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
#include "mask.h"
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
#ifdef RF_RX
uint8_t ghl_mask[RF_COUNT][sizeof(PS3GHLGuitar_Data_t)];
uint8_t standard_mask[RF_COUNT][sizeof(combined_report_t)];
uint8_t keyboard_masks[RF_COUNT][3][sizeof(combined_report_t)];
#endif
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
USB_Report_Data_t lastReport;
long initialWt[5] = {0};
uint8_t rawWt;
#ifdef TICK_NKRO
USB_NKRO_Data_t lastNKROReport;
#endif
#ifdef TICK_MOUSE
USB_Mouse_Data_t lastMouseReport;
#endif
#ifdef TICK_CONSUMER
USB_ConsumerControl_Data_t lastConsumerReport;
#endif
#ifdef RF_TX
RfInputPacket_t rf_report = {Input};
RfHeartbeatPacket_t rf_heartbeat = {Heartbeat};
#endif
#ifdef RF
#include "SPI.h"
#include "rf.h"
RF24 radio(RADIO_CE, RADIO_CSN);
uint8_t rf_data[32];
bool rf_successful = false;
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
#ifdef RF_RX
uint64_t addresses[RF_COUNT] = RF_ADDRESSES;
void send_rf_console_type() {
    if (rf_successful) {
        RfConsoleTypePacket_t packet = {
            AckConsoleType, consoleType};
        radio.writeAckPayload(1, &packet, sizeof(packet));
    }
}
#endif
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
    rf_successful = radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.enableDynamicPayloads();
    radio.enableAckPayload();
#endif
#ifdef RF_TX
    radio.openWritingPipe((uint64_t)TRANSMIT_RADIO_ID);  // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, (uint64_t)DEST_RADIO_ID);
    radio.stopListening();
#endif
#ifdef RF_RX
    radio.openWritingPipe((uint64_t)DEST_RADIO_ID);  // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    for (int i = 0; i < RF_COUNT; i++) {
        radio.openReadingPipe(i + 1, addresses[i]);
    }
    radio.startListening();
    send_rf_console_type();
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
    int16_t ret = handle_calibration_xbox(orig_val, offset, min, multiplier, deadzone) >> 8;
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
uint8_t tick_inputs(uint8_t *buf) {
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
            lastReportToCheck = &lastMouseReport;
        }
#endif
#ifdef TICK_CONSUMER
        if (i == REPORT_ID_CONSUMER) {
            size = sizeof(USB_ConsumerControl_Data_t);
            memset(buf, 0, size);
            USB_ConsumerControl_Data_t *report = (USB_ConsumerControl_Data_t *)buf;
            report->rid = REPORT_ID_CONSUMER;
            TICK_CONSUMER;
            lastReportToCheck = &lastConsumerReport;
        }
#endif
#ifdef TICK_NKRO
        if (i == REPORT_ID_NKRO) {
            size = sizeof(USB_NKRO_Data_t);
            memset(buf, 0, size);
            USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)buf;
            report->rid = REPORT_ID_NKRO;
            TICK_NKRO;
            lastReportToCheck = &lastNKROReport;
        }
#endif
        uint8_t cmp = memcmp(lastReportToCheck, buf, size);
        if (cmp == 0) {
            size = 0;
            continue;
        }
        memcpy(lastReportToCheck, buf, size);
        break;
    }
#if BLUETOOTH
    send_report(size, (uint8_t *)buf);
#endif
#else
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
    if (consoleType == WINDOWS_XBOX360 || consoleType == STAGE_KIT) {
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
// DJ Hero was never on ps4, so we can't really implement that either. For both of these fall back to PS3.
#if SUPPORTS_PS4
    if (consoleType == PS4) {
        PS4_REPORT *report = (PS4_REPORT *)report_data;
        PS4Gamepad_Data_t *gamepad = (PS4Gamepad_Data_t *)report;
        gamepad->report_id = 0x01;
        gamepad->leftStickX = PS3_STICK_CENTER;
        gamepad->leftStickY = PS3_STICK_CENTER;
        gamepad->rightStickX = PS3_STICK_CENTER;
        gamepad->rightStickY = PS3_STICK_CENTER;
#if !SUPPORTS_INSTRUMENT
        gamepad->reportCounter = ps4_sequence_number;
#endif
        TICK_PS4;
        report->dpad = (report->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[report->dpad];
        report_size = size = sizeof(PS4_REPORT);
    } 
#endif
    if (consoleType != WINDOWS_XBOX360 && consoleType != PS4 && consoleType != STAGE_KIT && !updateHIDSequence) {
        PS3_REPORT *report = (PS3_REPORT *)report_data;
        memset(report, 0, sizeof(PS3_REPORT));
        // If we are in instrument mode, then set defaults using the PC gamepad data report, which is modelled after the instrument ones
#if SUPPORTS_INSTRUMENT
        PCGamepad_Data_t *gamepad = (PCGamepad_Data_t *)report;
#else
        PS3Gamepad_Data_t *gamepad = (PS3Gamepad_Data_t *)report;
#endif
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
// On anything that isnt a standard PS3 controller
#if SUPPORTS_INSTRUMENT
        gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
#endif
        // Switch swaps a and b
        if (consoleType == SWITCH) {
            bool a = report->a;
            bool b = report->b;
            report->b = a;
            report->a = b;
        }
        report_size = size = sizeof(PS3_REPORT);
    }
    // For bluetooth, tick it seperately
#if BLUETOOTH
    PS3_REPORT *report = &bt_report;
    memset(report, 0, sizeof(PS3_REPORT));
    // If we are in instrument mode, then set defaults using the PC gamepad data report, which is modelled after the instrument ones
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
#endif
    if (report_data != &combined_report) {
        return size;
    }
    uint8_t cmp = memcmp(&lastReport, report_data, report_size);
    if (cmp == 0) {
        return 0;
    }
    memcpy(&lastReport, report_data, report_size);
#if BLUETOOTH
    send_report(sizeof(PS3_REPORT), (uint8_t *)&bt_report);
#endif
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
#ifdef TICK_LED
    TICK_LED;
#endif
    if (millis() < 1000 && consoleType == UNIVERSAL) {
        TICK_DETECTION;
    }
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

void xinput_controller_connected(uint8_t vid, uint8_t pid) {
    if (xbox_360_state == Authenticated) return;
    xbox_360_vid = vid;
    xbox_360_pid = pid;
}

void xone_controller_connected(void) {
    if (xbox_one_state == Ready) return;

    GipPowerMode_t *powerMode = (GipPowerMode_t *)data_from_console;
    GIP_HEADER(powerMode, GIP_POWER_MODE_DEVICE_CONFIG, true, 1);
    powerMode->subcommand = 0x00;
    send_report_to_controller(data_from_console, sizeof(GipPowerMode_t));
}
void controller_disconnected(void) {
}

#ifdef RF_TX
void send_mask(void) {
    uint8_t size;
    RfMaskPacket_t mask;
    mask.packet_id = Mask;
#if CONSOLE_TYPE == KEYBOARD_MOUSE
    for (int i = 1; i < REPORT_ID_END; i++) {
        // set report id
        mask.mask[0] = i;
        memset(&mask, 0, sizeof(mask));
#ifdef TICK_MOUSE
        // report size + packet id
        if (i == REPORT_ID_MOUSE) {
            size = sizeof(USB_Mouse_Data_t);
            MOUSE_MASK;
            radio.write(&mask, size + 1);
        }
#endif
#ifdef TICK_CONSUMER
        // report size + packet id
        if (i == REPORT_ID_CONSUMER) {
            size = sizeof(USB_ConsumerControl_Data_t);
            CONSUMER_MASK;
            radio.write(&mask, size + 1);
        }
#endif
#ifdef TICK_NKRO
        // report size + packet id
        if (i == REPORT_ID_NKRO) {
            size = sizeof(USB_NKRO_Data_t);
            KEYBOARD_MASK;
            radio.write(&mask, size + 1);
        }
#endif
    }
#else
    memset(&mask, 0, sizeof(mask));
    if (consoleType == XBOXONE) {
        size = sizeof(XBOX_ONE_REPORT);
        XBOX_ONE_MASK;
        mask.mask[0] = GIP_INPUT_REPORT;
    } else if (consoleType == WINDOWS_XBOX360) {
        size = sizeof(XINPUT_REPORT);
        XINPUT_MASK;
    } else {
        size = sizeof(PS3_REPORT);
        PS3_MASK;
    }
    // report size + packet id
    radio.write(&mask, size + 1);
// GHL XB1 guitars have two seperate reports, so we need to send masks for both.
#if DEVICE_TYPE_IS_LIVE_GUITAR
    if (consoleType == XBOXONE) {
        memset(&mask, 0, sizeof(mask));
        size = sizeof(XboxOneGHLGuitar_Data_t);
        PS3_MASK;
        RfMaskPacketGHL_t ghl_mask;
        memcpy(ghl_mask.mask, mask.mask, sizeof(PS3GHLGuitar_Data_t));
        ghl_mask.header.command = GHL_HID_REPORT;
        nrfRadio.send(DEST_RADIO_ID, &mask, size + 1);
    }
#endif
#endif
}
#endif
#ifdef RF_RX
void parse_mask(uint8_t size, uint8_t pipe) {
    RfMaskPacket_t *mask = (RfMaskPacket_t *)rf_data;
#if CONSOLE_TYPE == KEYBOARD_MOUSE
    uint8_t report_id = mask->mask[0];
    mask->mask[0] = 0xFF;
    memcpy(keyboard_masks[mask->radio_id][report_id], mask->mask, size - 1);
#else
    if (consoleType == XBOXONE) {
        size = sizeof(XBOX_ONE_REPORT);
        if (mask->mask[0] == GIP_INPUT_REPORT) {
            mask->mask[0] = 0xFF;
            memcpy(standard_mask[pipe - 1], mask->mask, size - 1);
        } else if (mask->mask[0] == GHL_HID_REPORT) {
            mask->mask[0] = 0xFF;
            RfMaskPacketGHL_t *mask_ghl = (RfMaskPacketGHL_t *)rf_data;
            memcpy(ghl_mask[pipe - 1], mask_ghl->mask, size - 2);
        }
    } else if (consoleType == WINDOWS_XBOX360) {
        size = sizeof(XINPUT_REPORT);
        memcpy(standard_mask[pipe - 1], mask->mask, size - 1);
    } else {
        size = sizeof(PS3_REPORT);
        memcpy(standard_mask[pipe - 1], mask->mask, size - 1);
    }
#endif
}
#endif
void tick(void) {
    uint8_t size = 0;
#ifdef RF_TX
    uint8_t pipe;
    if (radio.available(&pipe)) {
        radio.read(rf_data, sizeof(rf_data));
        switch (rf_data[0]) {
            case AckConsoleType:
                consoleType = rf_data[1];
                send_mask();
                break;
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

#endif
#ifndef RF_ONLY
    // If we have something pending to send to the xbox one controller, send it
    if (data_from_console_size) {
        send_report_to_controller(data_from_console, data_from_console_size);
        data_from_console_size = 0;
    }
    bool ready = ready_for_next_packet();
    bool bluetooth_ready = BLUETOOTH && check_bluetooth_ready();
    if (consoleType == XBOXONE && xbox_one_state != Ready) {
        if (!ready) {
            return;
        }
        size = tick_xbox_one();
        send_report_to_pc(&combined_report, size);
        return;
    }
#else
    bool ready = true;
    bool bluetooth_ready = BLUETOOTH;
#endif
    // Tick the guitar every 5ms if usb is not ready
    if (ready || bluetooth_ready || millis() - lastSentPacket > 5) {
        lastSentPacket = millis();
#ifdef RF_RX
        uint8_t pipe;
        if (radio.available(&pipe)) {
            radio.read(rf_data, sizeof(rf_data));
            size = radio.getDynamicPayloadSize();
            if (rf_data[0] == Input) {
                RfInputPacket_t *input = (RfInputPacket_t *)rf_data;

#if CONSOLE_TYPE == KEYBOARD_MOUSE
                uint8_t report_id = mask->mask[0];
                uint8_t *mask = keyboard_masks[input->radio_id][report_id];
#else
                uint8_t *mask = standard_mask[pipe - 1];
                if (consoleType == XBOXONE) {
                    // GHL Guitars use a different mask
                    GipHeader_t *header = (GipHeader_t *)&combined_report;
                    if (header->command == GHL_HID_REPORT) {
                        mask = ghl_mask[pipe - 1];
                    }
                }
#endif

                for (int i = 1; i < size; i++) {
                    // Fliter out the masked bits from the current report
                    combined_report.raw[i - 1] &= ~mask[i - 1];
                    // And then write in the new bits
                    combined_report.raw[i - 1] |= input->data[i];
                }
            }
            if (rf_data[0] == Mask) {
                parse_mask(size, pipe);
                return;
            }
            if (consoleType == XBOXONE) {
                // Put together a new header, as the old one will be masked away and have the wrong sequence ids
                XBOX_ONE_REPORT *report = (XBOX_ONE_REPORT *)&combined_report;
                GipHeader_t *header = (GipHeader_t *)&combined_report;
                if (header->command == GIP_INPUT_REPORT) {
                    GIP_HEADER(report, GIP_INPUT_REPORT, false, report_sequence_number++);
                } else {
                    GIP_HEADER(report, GHL_HID_REPORT, false, hid_sequence_number++);
                }
            }
        }
#else
        size = tick_inputs((uint8_t *)&combined_report);

#endif

#ifdef RF_TX
        if (!usb_connected()) {
            if (size > 0) {
                memcpy(rf_report.data, &combined_report, size);
                radio.write(&rf_report, size + 1);
            } else {
                radio.write(&rf_heartbeat, sizeof(rf_heartbeat));
            }
            return;
        }
#endif

#ifndef RF_ONLY
        if (size && ready) {
            send_report_to_pc(&combined_report, size);
        }
#endif
    }
}