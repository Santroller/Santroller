#include "shared_main.h"

#include "Arduino.h"
#include "accel.h"
#include "bt.h"
#include "config.h"
#include "controllers.h"
#include "endpoints.h"
#include "fxpt_math.h"
#include "hid.h"
#include "inputs/slave.h"
#include "io.h"
#include "io_define.h"
#include "max170x.h"
#include "mpr121.h"
#include "pico_slave.h"
#include "pin_funcs.h"
#include "ps2.h"
#include "state_translation/states.h"
#include "usbhid.h"
#include "util.h"
#include "wii.h"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
#define GH5NECK_ADDR 0x0D
#define CLONE_ADDR 0x10
#define CLONE_VALID_PACKET 0x52
#define GH5NECK_BUTTONS_PTR 0x12
#define GH5DRUM_BUTTONS_PTR 0x10
#define BUFFER_SIZE_QUEUE 255
#define KEY_ERR_OVF 0x01
#define REQUIRE_LED_DEBOUNCE LED_COUNT || LED_COUNT_PERIPHERAL || LED_COUNT_STP || LED_COUNT_PERIPHERAL_STP || LED_COUNT_WS2812 || LED_COUNT_PERIPHERAL_WS2812 || HAS_LED_OUTPUT || LED_COUNT_MPR121 || LED_COUNT_WS2812W || LED_COUNT_PERIPHERAL_WS2812W

#ifdef INPUT_MIDI
Midi_Data_t midiData = {0};
void onNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    // velocities are 7 bit
    printf("Note ON ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    midiData.midiVelocities[note] = velocity << 1;
}

void offNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    printf("Note OFF ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    midiData.midiVelocities[note] = 0;
}

void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2) {
    // cc are 7 bit
    printf("ControlChange ch=%d, b1=%d, b2=%d\r\n", channel, b1, b2);
    if (b1 == MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL) {
        midiData.midiSustainPedal = b2 << 1;
    }
    if (b1 == MIDI_CONTROL_COMMAND_MOD_WHEEL) {
        midiData.midiModWheel = b2 << 1;
    }
}

void onPitchBend(uint8_t channel, int pitch) {
    // pitchbend is signed 14 bit
    printf("PitchBend ch=%d, pitch=%d\r\n", channel, pitch);
    midiData.midiPitchWheel = pitch << 2;
}
#endif
uint8_t tmp = 0;
long clone_guitar_timer = 0;
long clone_guitar_ready_timer = 0;
bool clone_ready = false;
bool reading = false;
Buffer_Report_t last_queue_report;
long last_queue = 0;
uint8_t brightness = LED_BRIGHTNESS;
uint8_t queue_size = 0;
uint8_t led_tmp;
uint8_t queue_tail = 0;
Buffer_Report_t queue[BUFFER_SIZE_QUEUE];
#define TURNTABLE_BUFFER_SIZE 16
#if INPUT_DJ_TURNTABLE_SMOOTHING
int16_t dj_sum_left = 0;
int8_t dj_last_readings_left[TURNTABLE_BUFFER_SIZE];
int8_t dj_next_left = 0;
int16_t dj_sum_right = 0;
int8_t dj_last_readings_right[TURNTABLE_BUFFER_SIZE];
int8_t dj_next_right = 0;
#endif
USB_Host_Data_t wii_report;
USB_Host_Data_t ps2_report;
USB_Host_Data_t last_report;
USB_Report_Data_t bt_report;
uint8_t debounce[DIGITAL_COUNT];
uint8_t ledDebounce[LED_DEBOUNCE_COUNT];
uint16_t lastDrum[DIGITAL_COUNT];
uint8_t drumVelocity[8];
bool tiltActive = false;
long lastTilt = 0;
long lastDj = 0;
long lastSentPacket = 0;
long lastLed = 0;
long lastSentGHLPoke = 0;
long input_start = 0;
long lastDebounce = 0;
uint16_t lastMpr121 = 0;
bool hasTapBar = false;
uint8_t ghl_sequence_number_host = 1;
uint16_t wiiControllerType = WII_NO_EXTENSION;
uint8_t ps2ControllerType = PSX_NO_DEVICE;
uint8_t lastSuccessfulPS2Packet[BUFFER_SIZE];
uint8_t lastSuccessfulWiiPacket[8];
uint8_t lastSuccessfulGH5Packet[2];
uint8_t lastSuccessfulClonePacket[4];
uint8_t lastSuccessfulTurntablePacketLeft[3];
uint8_t lastSuccessfulTurntablePacketRight[3];
long lastSuccessfulGHWTPacket;
bool lastGH5WasSuccessful = false;
bool lastCloneWasSuccessful = false;
bool lastTurntableWasSuccessfulLeft = false;
bool lastTurntableWasSuccessfulRight = false;
bool lastWiiWasSuccessful = false;
bool lastPS2WasSuccessful = false;
bool overrideR2 = false;
bool lastXboxOneGuide = false;
bool disable_multiplexer = false;
uint8_t overriddenR2 = 0;
USB_LastReport_Data_t last_report_usb;
USB_LastReport_Data_t last_report_bt;
uint8_t wii_data[8];
#ifdef INPUT_USB_HOST
uint8_t temp_report_usb_host[128];
USB_Host_Data_t last_usb_host_data;
#endif
#if BLUETOOTH_RX
USB_Host_Data_t bt_data;
#endif
uint8_t rawWt;
uint8_t rawWtPeripheral;
bool auth_ps4_controller_found = false;
bool auth_ps4_is_ghl = false;
bool seen_ps4_console = false;
bool hasFlags = false;
GipPowerMode_t powerMode;
Gip_Led_On_t ledOn;
Gip_Auth_Done_t authDonePacket;
long last_poll = 0;
long last_poll_dj_ps3 = 0;
bool festival_gameplay_mode = false;
long last_festival_toggle = 0;
/* Magic data taken from GHLtarUtility:
 * https://github.com/ghlre/GHLtarUtility/blob/master/PS3Guitar.cs
 * Note: The Wii U and PS3 dongles happen to share the same!
 */
uint8_t ghl_ps3wiiu_magic_data[] = {
    0x02, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00};

/* Magic data for the PS4 dongles sniffed with a USB protocol
 * analyzer.
 */
uint8_t ghl_ps4_magic_data[] = {
    0x30, 0x02, 0x08, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t clone_data[] = {0x53, 0x10, 0x00, 0x01};
uint8_t ws2812_bits[] = {0x88, 0x8C, 0xC8, 0xCC};
#if LED_COUNT_WS2812W
Led_WS2812W_t ledState[LED_COUNT_WS2812W];
Led_WS2812W_t lastLedState[LED_COUNT_WS2812W];
#elif LED_COUNT_WS2812
Led_WS2812_t ledState[LED_COUNT_WS2812];
Led_WS2812_t lastLedState[LED_COUNT_WS2812];
#elif LED_COUNT_STP
uint8_t ledState[ROUND_UP(LED_COUNT_STP, 8) / 8];
uint8_t ledStateSelect[ROUND_UP(LED_COUNT_STP, 8) / 8];
uint8_t lastLedState[ROUND_UP(LED_COUNT_STP, 8) / 8];
#else
Led_t ledState[LED_COUNT];
Led_t lastLedState[LED_COUNT];
#endif
#if LED_COUNT_PERIPHERAL_WS2812W
Led_WS2812W_t ledStatePeripheral[LED_COUNT_PERIPHERAL_WS2812W];
Led_WS2812W_t lastLedStatePeripheral[LED_COUNT_PERIPHERAL_WS2812W];
#elif LED_COUNT_PERIPHERAL_WS2812
Led_WS2812_t ledStatePeripheral[LED_COUNT_PERIPHERAL_WS2812];
Led_WS2812_t lastLedStatePeripheral[LED_COUNT_PERIPHERAL_WS2812];
#elif LED_COUNT_PERIPHERAL_STP
uint8_t ledStatePeripheral[ROUND_UP(LED_COUNT_PERIPHERAL_STP, 8) / 8];
uint8_t ledStatePeripheralSelect[ROUND_UP(LED_COUNT_PERIPHERAL_STP, 8) / 8];
uint8_t lastLedStatePeripheral[ROUND_UP(LED_COUNT_PERIPHERAL_STP, 8) / 8];
#else
Led_t ledStatePeripheral[LED_COUNT_PERIPHERAL];
Led_t lastLedStatePeripheral[LED_COUNT_PERIPHERAL];
#endif
#if LED_COUNT_MPR121
uint8_t ledStateMpr121 = 0;
uint8_t ledStateMpr121Select = 0;
uint8_t lastLedStateMpr121 = 0;
#endif
const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
const uint8_t dpad_bindings_reverse[] = {UP, UP | RIGHT, RIGHT, DOWN | RIGHT, DOWN, DOWN | LEFT, LEFT, UP | LEFT};

uint8_t gh5_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};
void setKey(uint8_t id, uint8_t key, USB_6KRO_Data_t *report, bool state) {
    if (state) {
        for (size_t i = 0; i < 6; i++) {
            if (report->KeyCode[i] == key) {
                return;
            }
            if (!report->KeyCode[i]) {
                report->KeyCode[i] = key;
                return;
            }
        }
        // Too many buttons - flag the overflow condition and clear all key indexes
        memset(report->KeyCode, KEY_ERR_OVF, sizeof(report->KeyCode));
        return;
    }
}
uint8_t seq = 1;
void init_main(void) {
#if !DEVICE_TYPE_IS_GAMEPAD
    consoleType = UNIVERSAL;
#endif
    initPins();
    twi_init();
    spi_begin();
    GIP_HEADER((&powerMode), GIP_POWER_MODE_DEVICE_CONFIG, true, 1);
    powerMode.subcommand = GIP_POWER_ON;
    GIP_HEADER((&ledOn), GIP_CMD_LED, true, 1);
    ledOn.Header.sequence = 1;
    ledOn.mode = GIP_LED_ON;
    ledOn.brightness = 0x14;
    ledOn.unk = 0;
    GIP_HEADER((&authDonePacket), GIP_AUTHENTICATION, true, 1);
    authDonePacket.unk1 = 0x01;
    authDonePacket.unk2 = 0x00;
    authDonePacket.Header.sequence = 2;
    memset(ledState, 0, sizeof(ledState));
    memset(ledStatePeripheral, 0, sizeof(ledStatePeripheral));
    LED_INIT;
#if INPUT_DJ_TURNTABLE_SMOOTHING
    memset(dj_last_readings_left, 0, sizeof(dj_last_readings_left));
    memset(dj_last_readings_right, 0, sizeof(dj_last_readings_right));
#endif
#ifdef INPUT_PS2
    init_ack();
#endif
#ifdef TICK_PS2
    ps2_emu_init();
#endif
#ifdef INPUT_MIDI
    memset(midiData.midiVelocities, 0, sizeof(midiData.midiVelocities));
#endif
#ifdef TICK_WII
    initWiiOutput();
#endif
}
#ifdef SLAVE_TWI_PORT
bool slave_initted = false;
void tick_slave() {
    if (slave_initted) {
        return;
    }
    if (!slaveInit()) {
        return;
    }
    slave_initted = true;
    PIN_INIT_PERIPHERAL;
#ifdef SPI_SLAVE_0_MOSI
    slaveInitLED(0);
    slavePinMode(SPI_SLAVE_0_MOSI, PIN_MODE_SPI);
    slavePinMode(SPI_SLAVE_0_SCK, PIN_MODE_SPI);
#endif
#ifdef SPI_SLAVE_1_MOSI
    slaveInitLED(1);
    slavePinMode(SPI_SLAVE_1_MOSI, PIN_MODE_SPI);
    slavePinMode(SPI_SLAVE_1_SCK, PIN_MODE_SPI);
#endif
#ifdef INPUT_WT_SLAVE_NECK
    slaveInitWt();
#endif
}
#endif
int16_t adc_i(uint8_t pin) {
    int32_t ret = adc(pin);
    return ret - 32767;
}
int16_t handle_calibration_xbox(int16_t previous, int16_t orig_val, int16_t min, int16_t max, int16_t center, int16_t deadzone) {
    int32_t val = orig_val;
    if (val < center) {
        if ((center - val) < deadzone) {
            val = 0;
        } else {
            val = map(val, min, center - deadzone, INT16_MIN, 0);
        }

    } else {
        if ((val - center) < deadzone) {
            val = 0;
        } else {
            val = map(val, center + deadzone, max, 0, INT16_MAX);
        }
    }
    if (val > INT16_MAX) {
        val = INT16_MAX;
    }
    if (val < INT16_MIN) {
        val = INT16_MIN;
    }
    if (abs(val) > abs(previous)) {
        return val;
    }
    return previous;
}

int16_t handle_calibration_xbox_whammy(int16_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone) {
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
    if (val > previous) {
        return val;
    }
    return previous;
}
uint16_t handle_calibration_xbox_one_trigger(uint16_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone) {
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
    val = val >> 6;
    if (val > previous) {
        return val;
    }
    return previous;
}
uint16_t handle_calibration_drum(uint16_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone) {
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
    if (val > previous) {
        return val;
    }
    return previous;
}
uint8_t handle_calibration_ps3(uint8_t previous, int16_t orig_val, int16_t min, int16_t max, int16_t center, int16_t deadzone) {
    int8_t ret = handle_calibration_xbox((previous - PS3_STICK_CENTER) << 8, orig_val, min, max, center, deadzone) >> 8;
    return (uint8_t)(ret + PS3_STICK_CENTER);
}

int8_t handle_calibration_mouse(int8_t previous, int16_t orig_val, int16_t min, int16_t max, int16_t center, int16_t deadzone) {
    return handle_calibration_xbox(previous << 8, orig_val, min, max, center, deadzone) >> 8;
}

uint8_t handle_calibration_ps3_360_trigger(uint8_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone) {
    return handle_calibration_xbox_one_trigger(previous << 2, orig_val, min, multiplier, deadzone) >> 2;
}

uint16_t handle_calibration_ps3_accel(uint16_t previous, uint16_t orig_val, int16_t offset, uint32_t min, int16_t multiplier, uint16_t deadzone) {
#if DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE_IS_LIVE_GUITAR
    // For whatever reason, the acceleration for guitars swings between -128 to 128, not -512 to 512
    // Also, the game is looking for the value going negative, not positive
    int16_t ret = (-(handle_calibration_xbox((-(previous + GUITAR_ONE_G)) << 7, orig_val, offset, min, multiplier, deadzone) >> 7)) - GUITAR_ONE_G;
#else
    int16_t ret = handle_calibration_xbox((previous - PS3_ACCEL_CENTER) << 6, orig_val, offset, min, multiplier, deadzone) >> 6;
#endif
    return PS3_ACCEL_CENTER + ret;
}
long last_zero = 0;
uint8_t handle_calibration_ps3_whammy(uint8_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone) {
#if DEVICE_TYPE == ROCK_BAND_GUITAR
    // RB whammy is full range
    return handle_calibration_ps3_360_trigger(previous, orig_val, min, multiplier, deadzone);
#else
    // GH whammy ignores the negative half of the axis, so shift to get between 0 and 127, then add center
    uint8_t ret = handle_calibration_ps3_360_trigger(previous << 1, orig_val, min, multiplier, deadzone) >> 1;
    return ret + PS3_STICK_CENTER;
#endif
}
uint8_t handle_calibration_ps2_whammy(uint8_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone) {
    // GH whammy ignores the negative half of the axis, so shift to get between 0 and 127, then add center
    uint8_t ret = handle_calibration_ps3_360_trigger(previous << 1, orig_val, min, multiplier, deadzone) >> 1;
    return 0x7f - ret;
}

uint8_t handle_calibration_turntable_ps3(uint8_t previous, int16_t orig_val, uint8_t multiplier) {
    int32_t val = (orig_val * multiplier) >> 8;
    if (val > INT8_MAX) {
        val = INT8_MAX;
    }
    if (val < INT8_MIN) {
        val = INT8_MIN;
    }
    // previous is uint, convert to int to test
    if (abs(val) > abs((int)previous - PS3_STICK_CENTER)) {
        // ps3 wants uint, convert to uint
        return (uint8_t)(val + PS3_STICK_CENTER);
    }
    return previous;
}
int16_t handle_calibration_turntable_360(int16_t previous, int16_t orig_val, uint8_t multiplier) {
    if (consoleType == WINDOWS && WINDOWS_TURNTABLE_FULLRANGE) {
        int32_t val = (orig_val * multiplier);
        if (val > INT16_MAX) {
            val = INT16_MAX;
        }
        if (val < INT16_MIN) {
            val = INT16_MIN;
        }
        if (abs(val) > abs(previous)) {
            return (int16_t)val;
        }
        return previous;
    }
    int32_t val = (orig_val * multiplier) >> 8;
    if (val > 128) {
        val = 128;
    }
    if (val < -128) {
        val = -128;
    }
    if (abs(val) > abs(previous)) {
        return (int16_t)val;
    }
    return previous;
}
uint16_t descriptor_index = 0;
uint8_t tick_xbox_one() {
    uint32_t serial = micros();
    const uint8_t *announce;
    const uint8_t *xb1_descriptor;
    const uint8_t *xb1_descriptor_end;
    uint16_t len;
    switch (DEVICE_TYPE) {
        case ROCK_BAND_DRUMS:
        case GUITAR_HERO_DRUMS:
            announce = announce_drums;
            xb1_descriptor = xb1_descriptor_drums;
            xb1_descriptor_end = xb1_descriptor_drums_end;
            len = sizeof(xb1_descriptor_drums);
            break;
        case GUITAR_HERO_GUITAR:
        case ROCK_BAND_GUITAR:
            announce = announce_guitar;
            xb1_descriptor = xb1_descriptor_guitar;
            xb1_descriptor_end = xb1_descriptor_guitar_end;
            len = sizeof(xb1_descriptor_guitar);
            break;
        case LIVE_GUITAR:
            announce = announce_ghl;
            xb1_descriptor = xb1_descriptor_ghl;
            xb1_descriptor_end = xb1_descriptor_ghl_end;
            len = sizeof(xb1_descriptor_ghl);
            break;
        default:
            announce = announce_gamepad;
            xb1_descriptor = xb1_descriptor_gamepad;
            xb1_descriptor_end = xb1_descriptor_gamepad_end;
            len = sizeof(xb1_descriptor_gamepad);
            break;
    }
    switch (xbox_one_state) {
        case Announce:
            descriptor_index = 0;
            xbox_one_state = WaitingDesc1;
            memcpy(&last_report, announce, sizeof(announce));
            memcpy(((uint8_t *)&last_report) + 7, &serial, 3);
            return sizeof(announce);
        case IdentDesc1:
            xbox_one_state = WaitingDesc;
            descriptor_index += 64;
            memcpy(&last_report, xb1_descriptor, 64);
            return 64;
        case IdentDesc: {
            uint16_t len = sizeof(xb1_descriptor) - descriptor_index;
            if (len > 64) {
                len = 64;
            } else {
                xbox_one_state = WaitingDescEnd;
            }
            memcpy(&last_report, xb1_descriptor + descriptor_index, len);
            descriptor_index += len;
            return len;
        }
        case IdentDescEnd: {
            xbox_one_state = Auth;
            memcpy(&last_report, xb1_descriptor_end, sizeof(xb1_descriptor_end));
            return sizeof(xb1_descriptor_end);
        }
        case Auth:
            if (data_from_controller_size) {
                uint8_t size = data_from_controller_size;
                data_from_controller_size = 0;
                memcpy(&last_report, data_from_controller, size);
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
void convert_report(const uint8_t *data, uint8_t len, uint8_t console_type, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    switch (console_type) {
        case PS2: {
            ps2_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case STREAM_DECK: {
            uint8_t offset = 1;
            switch (sub_type) {
                case STREAM_DECK_XL:
                case STREAM_DECK_MK2:
                case STREAM_DECK_NEO:
                case STREAM_DECK_V2:
                case STREAM_DECK_XLV2:
                case STREAM_DECK_PEDAL:
                case STREAM_DECK_PLUS:
                    offset = 4;
                    break;
            }
            for (uint8_t i = 0; i + offset < len && i < 16; i++) {
                bit_write(data[i + offset], usb_host_data->genericButtons, i);
            }
            break;
        }
        case KEYBOARD: {
            keyboard_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case MOUSE: {
            mouse_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case GENERIC: {
            generic_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case RAPHNET: {
            raphnet_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case SANTROLLER: {
            santroller_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case PS3: {
            ps3_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case LTEK_ID: {
            LTEK_Report_With_Id_Data_t *report = (LTEK_Report_With_Id_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
            usb_host_data->start |= report->start;
            usb_host_data->back |= report->back;
            break;
        }
        case LTEK: {
            LTEK_Report_Data_t *report = (LTEK_Report_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
            usb_host_data->start |= report->start;
            usb_host_data->back |= report->back;
            break;
        }
        case STEPMANIAX: {
            StepManiaX_Report_Data_t *report = (StepManiaX_Report_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
            break;
        }
        case PS4: {
            ps4_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case PS5: {
            ps5_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case SWITCH: {
            switch_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case XBOX360_BB: {
            x360bb_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case XBOX360_W:
        case XBOX360: {
            x360_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
        case XBOXONE: {
            xbone_to_universal_report(data, len, sub_type, usb_host_data);
            break;
        }
    }
}

uint8_t convert_report_back(uint8_t *data, uint8_t len, uint8_t console_type, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    uint8_t dpad = 0;
    if (usb_host_data->dpadUp) {
        dpad |= UP;
    }
    if (usb_host_data->dpadLeft) {
        dpad |= LEFT;
    }
    if (usb_host_data->dpadDown) {
        dpad |= DOWN;
    }
    if (usb_host_data->dpadRight) {
        dpad |= RIGHT;
    }
    // Cymbal flags are mapped to dpad up and down
    if (sub_type == ROCK_BAND_DRUMS) {
        if (usb_host_data->yellowCymbal) {
            dpad |= UP;
        }
        if (usb_host_data->blueCymbal) {
            dpad |= DOWN;
        }
    }
    dpad = dpad_bindings[dpad];
    switch (console_type) {
        case OG_XBOX: {
            return universal_report_to_ogxbox(data, len, sub_type, usb_host_data);
        }
        case BLUETOOTH_REPORT:
        case SANTROLLER: {
            return universal_report_to_santroller(dpad, data, len, sub_type, usb_host_data);
        }
#ifdef TICK_PS2
        case PS2: {
            return universal_report_to_ps2(data, len, sub_type, usb_host_data);
        }
#endif
#ifdef TICK_WII
        case WII: {
            return universal_report_to_wii(data, len, sub_type, usb_host_data);
        }
#endif
        case SWITCH:
        case PS3: {
            return universal_report_to_ps3(dpad, data, len, console_type, sub_type, usb_host_data);
        }
        case PS4: {
            return universal_report_to_ps4(dpad, data, len, sub_type, usb_host_data);
        }
        case PS5: {
            return universal_report_to_ps5(dpad, data, len, sub_type, usb_host_data);
        }
        case XBOX360_W:
        case XBOX360: {
            return universal_report_to_x360(data, len, sub_type, usb_host_data);
        }
        case XBOXONE: {
            return universal_report_to_xbone(data, len, sub_type, usb_host_data);
        }
    }
    return 0;
}

#ifdef TICK_PS2
PS2_REPORT ps2Report;
void tick_ps2output() {
    PS2_REPORT *report = &ps2Report;
    if (!ps2_emu_tick(report)) {
        return;
    }
    uint8_t packet_size = 0;
    Buffer_Report_t current_queue_report = {val : 0};
// Tick Inputs
#include "inputs/accel.h"
#include "inputs/clone_neck.h"
#include "inputs/gh5_neck.h"
#include "inputs/mpr121.h"
#include "inputs/ps2.h"
#include "inputs/slave_tick.h"
#include "inputs/turntable.h"
#include "inputs/usb_host.h"
#include "inputs/wii.h"
#include "inputs/wt_neck.h"

    TICK_SHARED;
    memset(report, 0, sizeof(report));
    TICK_PS2;
    report->header = 0x5A;
#if DEVICE_TYPE_IS_GUITAR
    report->dpadLeft = true;
#endif
}
#endif
#ifdef TICK_WII
void tick_wiioutput() {
#include "inputs/accel.h"
#include "inputs/clone_neck.h"
#include "inputs/gh5_neck.h"
#include "inputs/mpr121.h"
#include "inputs/ps2.h"
#include "inputs/slave_tick.h"
#include "inputs/turntable.h"
#include "inputs/usb_host.h"
#include "inputs/wii.h"
#include "inputs/wt_neck.h"
    TICK_SHARED;
#if DEVICE_TYPE_IS_GUITAR
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)wii_data;
    memset(wii_data, 0, sizeof(wii_data));
    // Center sticks
    report->leftStickX = 0x20;
    report->leftStickY = 0x20;
    report->slider = 0x0F;
    TICK_WII;
    wii_data[4] = ~wii_data[4];
    wii_data[5] = ~wii_data[5];
    setInputs(wii_data, sizeof(wii_data));
#elif DEVICE_TYPE_IS_DRUM
    WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)wii_data;
    memset(wii_data, 0, sizeof(wii_data));
    // Center sticks
    report->leftStickX = 0x20;
    report->leftStickY = 0x20;
    TICK_WII;
    wii_data[4] = ~wii_data[4];
    wii_data[5] = ~wii_data[5];
    setInputs(wii_data, sizeof(wii_data));
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
    WiiTurntableIntermediateFormat3_t temp_report;
    WiiTurntableIntermediateFormat3_t *report = &temp_report;
    // Center sticks
    report->leftStickX = 0x20;
    report->leftStickY = 0x20;
    memset(report, 0, sizeof(temp_report));
    memset(wii_data, 0, sizeof(wii_data));
    TICK_WII;
    // Turntable report format is so muddled that we have to transform things by hand after
    WiiTurntableDataFormat3_t *real_report = (WiiTurntableDataFormat3_t *)wii_data;
    // Button bits are inverted
    real_report->buttonsLow = ~report->buttonsLow;
    real_report->buttonsHigh = ~report->buttonsHigh;
    real_report->leftStickX = report->leftStickX;
    real_report->leftStickY = report->leftStickY;
    real_report->crossfader = report->crossfader;
    real_report->effectsKnob20 = report->effectsKnob20;
    real_report->effectsKnob43 = report->effectsKnob43;
    real_report->leftTableVelocity40 = report->leftTableVelocity40;
    real_report->leftTableVelocity5 = report->leftTableVelocity5;
    real_report->rightTableVelocity0 = report->rightTableVelocity0;
    real_report->rightTableVelocity21 = report->rightTableVelocity21;
    real_report->rightTableVelocity43 = report->rightTableVelocity43;
    real_report->rightTableVelocity5 = report->rightTableVelocity5;

    setInputs(wii_data, sizeof(wii_data));
#else
    // Report format 3 is reasonable, so we can convert from that
    WiiClassicDataFormat3_t temp_report;
    WiiClassicDataFormat3_t *report = &temp_report;
    memset(report, 0, sizeof(temp_report));
    memset(wii_data, 0, sizeof(wii_data));
    // Center sticks
    report->leftStickX = PS3_STICK_CENTER;
    report->leftStickY = PS3_STICK_CENTER;
    report->rightStickX = PS3_STICK_CENTER;
    report->rightStickY = PS3_STICK_CENTER;
    TICK_WII;
    // button bits are inverted
    report->buttonsLow = ~report->buttonsLow;
    report->buttonsHigh = ~report->buttonsHigh;
    uint8_t format = wii_data_format();
    if (format == 3) {
        memcpy(wii_data, report, sizeof(report));
    } else if (format == 2) {
        WiiClassicDataFormat2_t *real_report = (WiiClassicDataFormat2_t *)wii_data;
        real_report->buttonsLow = report->buttonsLow;
        real_report->buttonsHigh = report->buttonsHigh;
        real_report->leftStickX92 = report->leftStickX;
        real_report->leftStickY92 = report->leftStickY;
        real_report->rightStickX92 = report->rightStickX;
        real_report->rightStickY92 = report->rightStickY;
        real_report->leftTrigger = report->leftTrigger;
        real_report->rightTrigger = report->rightTrigger;
    } else if (format == 1) {
        // Similar to the turntable, classic format 1 is awful so we need to translate to an intermediate format first
        WiiIntermediateClassicDataFormat_t intermediate_report;
        intermediate_report.rightStickX = report->rightStickX >> 3;
        intermediate_report.leftTrigger = report->leftTrigger >> 3;
        WiiClassicDataFormat1_t *real_report = (WiiClassicDataFormat1_t *)wii_data;
        real_report->buttonsLow = report->buttonsLow;
        real_report->buttonsHigh = report->buttonsHigh;
        real_report->leftStickX = report->leftStickX >> 2;
        real_report->leftStickY = report->leftStickY >> 2;
        real_report->rightTrigger = report->rightTrigger >> 3;
        real_report->rightStickX0 = intermediate_report.rightStickX0;
        real_report->rightStickX21 = intermediate_report.rightStickX21;
        real_report->rightStickX43 = intermediate_report.rightStickX43;
        real_report->leftTrigger20 = intermediate_report.leftTrigger20;
        real_report->leftTrigger43 = intermediate_report.leftTrigger43;
    }
    setInputs(wii_data, sizeof(wii_data));
#endif
}
#endif
void bluetooth_connected() {
    input_start = millis();
}
uint8_t tick_keyboard(void *buf, USB_LastReport_Data_t *last_report) {
    // Tick all three reports, and then go for the first one that has changes
    // We prioritise NKRO, then Consumer, because these are both only buttons
    // Then mouse, as it is an axis so it is more likley to have changes
    uint8_t packet_size = 0;
    void *lastReportToCheck;
    for (int i = 1; i < REPORT_ID_END; i++) {
#ifdef TICK_MOUSE
        if (i == REPORT_ID_MOUSE) {
            packet_size = sizeof(USB_Mouse_Data_t);
            memset(buf, 0, packet_size);
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
            packet_size = sizeof(USB_ConsumerControl_Data_t);
            memset(buf, 0, packet_size);
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
            packet_size = sizeof(USB_NKRO_Data_t);
            memset(buf, 0, packet_size);
            USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)buf;
            report->rid = REPORT_ID_NKRO;
            TICK_NKRO;
            if (last_report) {
                lastReportToCheck = &last_report->lastNKROReport;
            }
        }
#endif
#ifdef TICK_SIXKRO
        if (i == REPORT_ID_NKRO) {
            packet_size = sizeof(USB_6KRO_Data_t);
            memset(buf, 0, packet_size);
            USB_6KRO_Data_t *report = (USB_6KRO_Data_t *)buf;
            report->rid = REPORT_ID_NKRO;
            TICK_SIXKRO;
            if (last_report) {
                lastReportToCheck = &last_report->last6KROReport;
            }
        }
#endif

        // If we are directly asked for a HID report, always just reply with the NKRO one
        if (lastReportToCheck) {
            uint8_t cmp = memcmp(lastReportToCheck, buf, packet_size);
            if (cmp == 0) {
                packet_size = 0;
                continue;
            }
            memcpy(lastReportToCheck, buf, packet_size);
            break;
        } else {
            break;
        }
    }
    return packet_size;
}
uint8_t tick_controllers(void *buf, USB_LastReport_Data_t *last_report, uint8_t output_console_type, USB_Host_Data_t *universal_report) {
    uint8_t current_mode = DEVICE_TYPE;
    if (festival_gameplay_mode) {
        current_mode = FORTNITE_GUITAR;
    }
    USB_Report_Data_t *report_data = (USB_Report_Data_t *)buf;
    uint8_t report_size;
    uint8_t packet_size = 0;
    bool updateSequence = false;
    bool updateHIDSequence = false;
    if (output_console_type == XBOXONE) {
        XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)buf;
        XboxOneGHLGuitarWithGamepad_Data_t *reportGHL = (XboxOneGHLGuitarWithGamepad_Data_t *)buf;
        // The GHL guitar is special. It sends both a standard gamepad report and a report with a format mirroring the ps3 ghl report
        // We append the standard gamepad after the ps3 report, so swap out buffers so that the gamepad tick writes to the correct place
        memset(buf, 0, sizeof(XboxOneGHLGuitarWithGamepad_Data_t));
        report = &reportGHL->gamepad;
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, XBOXONE, current_mode, universal_report);
        report_size = packet_size - sizeof(GipHeader_t);
        GIP_HEADER(report, GIP_INPUT_REPORT, false, report_sequence_number);

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
        GipPacket_t *packet = (GipPacket_t *)report;
        report_data = (USB_Report_Data_t *)packet->data;
        updateSequence = true;
        if (current_mode == LIVE_GUITAR) {
            packet_size = sizeof(XboxOneGHLGuitarWithGamepad_Data_t);
            uint8_t cmp = memcmp(last_report, report_data, report_size);
            if (cmp == 0) {
                // gamepad report has not changed, so only send GHL report
                packet_size = sizeof(XboxOneGHLGuitar_Data_t);
                updateSequence = false;
            }
            memcpy(last_report, report_data, report_size);
            GIP_HEADER((&reportGHL->guitar), GHL_HID_REPORT, false, hid_sequence_number);
            report_data = (USB_Report_Data_t *)&reportGHL->guitar.report;
            updateHIDSequence = true;
            report_size = convert_report_back((uint8_t *)report_data, 0, PS3, current_mode, universal_report);
        }
    }
    if (output_console_type == OG_XBOX) {
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, OG_XBOX, current_mode, universal_report);
    }
    if (output_console_type == WINDOWS || output_console_type == XBOX360) {
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, XBOX360, current_mode, universal_report);
    }
    // Guitars and Drums can fall back to their PS3 versions, so don't even include the PS4 version there.
    // DJ Hero was never on ps4, so we can't really implement that either, so just fall back to PS3 there too.
    if (output_console_type == PS4) {
        if (millis() > 450000 && !auth_ps4_controller_found) {
            reset_usb();
        }
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, PS4, current_mode, universal_report);
        PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)buf;
        // PS4 does not start using the controller until it sees a PS button press.
        if (!seen_ps4_console) {
            gamepad->guide = true;
        }
    }
    // IOS works native with PS3 gamepads in FNF so thats what we emulate for that scenario
    if (output_console_type == IOS_FESTIVAL) {
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, IOS_FESTIVAL, current_mode, universal_report);
    }
    if (output_console_type == BLUETOOTH_REPORT || output_console_type == UNIVERSAL) {
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, UNIVERSAL, current_mode, universal_report);
    }

    if (output_console_type == FNF) {
        convert_report_back((uint8_t *)buf, 0, FNF, current_mode, universal_report);
    }

    // For instruments, we instead use the below block, as all other console types use the below format
    if ((output_console_type == PS3 || output_console_type == WII || output_console_type == SWITCH)) {
        report_size = packet_size = convert_report_back((uint8_t *)buf, 0, PS3, current_mode, universal_report);
    }

    TICK_RESET
    // Some hosts want packets sent every frame
    if (last_report && output_console_type != OG_XBOX && output_console_type != PS4 && output_console_type != IOS_FESTIVAL && output_console_type != PS3 && output_console_type != BLUETOOTH_REPORT && output_console_type != XBOX360 && !updateHIDSequence) {
        uint8_t cmp = memcmp(last_report, report_data, report_size);
        if (cmp == 0) {
            return 0;
        }
        memcpy(last_report, report_data, report_size);
    }
    // Standard PS4 controllers need a report counter, but we don't want to include that when comparing so we add it here
    if (output_console_type == PS4) {
        PS4Gamepad_Data_t *gamepad = (PS4Gamepad_Data_t *)report_data;
        gamepad->reportCounter = ps4_sequence_number++;
    }

    if (updateSequence) {
        report_sequence_number++;
        if (report_sequence_number == 0) {
            report_sequence_number = 1;
        }
    }
    if (updateHIDSequence) {
        hid_sequence_number++;
        if (hid_sequence_number == 0) {
            hid_sequence_number = 1;
        }
    }
    return packet_size;
}
uint8_t rbcount = 0;
uint8_t tick_inputs(USB_Host_Data_t *report, USB_LastReport_Data_t *last_report, uint8_t output_console_type) {
    uint8_t packet_size = 0;
    Buffer_Report_t current_queue_report = {val : 0};
    USB_RB_Drums_t current_drum_report = {buttons : 0};
// Tick Inputs
#include "inputs/accel.h"
#include "inputs/clone_neck.h"
#include "inputs/gh5_neck.h"
#include "inputs/mpr121.h"
#include "inputs/slave_tick.h"
#include "inputs/turntable.h"
#include "inputs/usb_host.h"
#include "inputs/wt_neck.h"
#include "inputs/ps2.h"
#include "inputs/wii.h"
    TICK_SHARED;
    // give the user 2 second to jump between modes (aka, hold on plug in)
    if ((millis() - input_start) < 2000 && (output_console_type == UNIVERSAL || output_console_type == WINDOWS)) {
        TICK_DETECTION;
    }
#ifdef TICK_DETECTION_FESTIVAL
    TICK_DETECTION_FESTIVAL;
#endif
    // We tick the guitar every 5ms to handle inputs if nothing is attempting to read, but this doesn't need to output that data anywhere.
    // if input queues are enabled, then we just tick as often as possible
    if (!report) {
        if (INPUT_QUEUE) {
            if (micros() - last_queue > 100) {
                last_queue = micros();
                for (int i = 0; i < DIGITAL_COUNT; i++) {
                    if (debounce[i]) {
                        debounce[i]--;
                    }
                }
#if REQUIRE_LED_DEBOUNCE
                for (int i = 0; i < LED_DEBOUNCE_COUNT; i++) {
                    if (ledDebounce[i]) {
                        ledDebounce[i]--;
                    }
                }
#endif
            }
            if (current_queue_report.val != last_queue_report.val) {
                queue[queue_tail] = current_queue_report;
                last_queue_report = current_queue_report;
                if (queue_size < BUFFER_SIZE_QUEUE) {
                    queue_size++;
                    queue_tail++;
                }
            }
        }
        return 0;
    }

    if (INPUT_QUEUE && queue_size) {
        current_queue_report = queue[queue_tail - queue_size];
        queue_size--;
    }
    return packet_size;
}

#ifdef BLUETOOTH_TX
bool tick_bluetooth(void) {
    uint8_t size = tick_inputs(&bt_report, &last_report_bt, BLUETOOTH_REPORT);
    send_report(size, (uint8_t *)&bt_report);
    return size;
}
#endif
bool windows_in_hid = false;
unsigned long millis_at_boot = 0;
bool tick_usb(void) {
    uint8_t size = 0;
    bool ready = ready_for_next_packet();
#ifdef BLUETOOTH_TX
    if (!ready) {
        return false;
    }
#endif
    if (millis_at_boot == 0 && read_device_desc) {
        millis_at_boot = millis();
    }

#if DEVICE_TYPE_IS_GAMEPAD
    if (!WINDOWS_USES_XINPUT) {
        // If we ended up here, then someone probably changed back to hid mode so we should jump back
        if (consoleType == WINDOWS) {
            consoleType = UNIVERSAL;
            reset_usb();
        }
    }
    // PS2 / Wii / WiiU do not read the hid report descriptor or any of the string descriptors.
    if (millis_at_boot && (millis() - millis_at_boot) > 5000 && consoleType == UNIVERSAL && !seen_hid_descriptor_read && !read_any_string && !seen_windows_xb1) {
        // The wii however will configure the usb device before it stops communicating
        if (DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == ROCK_BAND_DRUMS) {
            if (usb_configured()) {
                set_console_type(WII_RB);
            }
        }
        // But the PS2 does not. We also end up here on the wii/wiiu if a device does not have an explicit wii mode.
        set_console_type(PS3);
    }
    // Due to some quirks with how the PS3 detects controllers, we can also end up here for PS3, but in that case, we won't see any requests for controller data
    if ((millis() - millis_at_boot) > 2000 && consoleType == PS4 && !seen_ps4) {
        set_console_type(PS3);
    }
#endif
    if (!ready) return 0;
#if USB_HOST_STACK
    if (data_from_console_size) {
        USB_Device_Type_t type = get_device_address_for(XBOXONE);
        send_report_to_controller(type.dev_addr, type.instance, data_from_console, data_from_console_size);
        data_from_console_size = 0;
    }
    // If we have something pending to send to the xbox one controller, send it
    if (consoleType == XBOXONE && xbox_one_state != Ready) {
        size = tick_xbox_one();
        if (!size) {
            // We don't want the controller ticked due to usb being stated as "not ready"
            return true;
        }
    }
#endif
    if (!size) {
        size = tick_inputs(&last_report, &last_report_usb, consoleType);
    }
    if (size) {
        send_report_to_pc(&last_report, size);
    }
    seen_ps4_console = true;
    return size;
}
#if BLUETOOTH_RX
int tick_bluetooth_inputs(const void *data, uint8_t len, USB_Device_Type_t device_type) {
    memset(&bt_data, 0, sizeof(bt_data));
    bt_data.slider = 0x80;
    convert_report((const uint8_t *)data, len, device_type.console_type, device_type.sub_type, &bt_data);
    return 0;
}
#endif
void tick(void) {
#ifdef SLAVE_TWI_PORT
    tick_slave();
#endif
#ifdef MAX1704X_TWI_PORT
    tick_max170x();
#endif
#ifdef TICK_LED_STROBE
    TICK_LED_STROBE;
#endif
#ifdef TICK_LED_BLUETOOTH
    TICK_LED_BLUETOOTH;
#endif
#ifdef TICK_LED_PERIPHERAL
    // If we are controlling peripheral leds, then we need to send the latest state when
    // the device is plugged in again
    if (slave_initted) {
        if (memcmp(lastLedStatePeripheral, ledStatePeripheral, sizeof(ledStatePeripheral)) != 0) {
            memcpy(lastLedStatePeripheral, ledStatePeripheral, sizeof(ledStatePeripheral));
            TICK_LED_PERIPHERAL;
        }
    } else {
        memset(lastLedStatePeripheral, 0, sizeof(lastLedStatePeripheral));
    }
#endif
#ifdef TICK_LED
    if (memcmp(lastLedState, ledState, sizeof(ledState)) != 0) {
        memcpy(lastLedState, ledState, sizeof(ledState));
        TICK_LED;
    }
#endif

#if LED_COUNT_MPR121
    if (lastLedStateMpr121 != ledStateMpr121) {
        lastLedStateMpr121 = ledStateMpr121;
        twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_GPIODATA, ledStateMpr121);
    }
#endif
#ifdef TICK_PS2
    tick_ps2output();
#endif
    if (!INPUT_QUEUE && micros() - lastDebounce > 1000) {
        // No benefit to ticking bluetooth faster than this!
#ifdef BLUETOOTH_TX
        tick_bluetooth();
#endif
        lastDebounce = micros();
        for (int i = 0; i < DIGITAL_COUNT; i++) {
            if (debounce[i]) {
                debounce[i]--;
            }
        }
#if REQUIRE_LED_DEBOUNCE
        for (int i = 0; i < LED_DEBOUNCE_COUNT; i++) {
            if (ledDebounce[i]) {
                ledDebounce[i]--;
            }
        }
#endif
    }
#if DEVICE_TYPE_IS_GUITAR
    if (consoleType == KEYBOARD_MOUSE || consoleType == FNF) {
        if (!INPUT_QUEUE && (micros() - last_poll) < (4000)) {
            return;
        }
    }
#endif
    if (!INPUT_QUEUE && POLL_RATE && (micros() - last_poll) < (POLL_RATE * 1000)) {
        return;
    }
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
    if (consoleType == PS3) {
        if (!INPUT_QUEUE && (micros() - last_poll_dj_ps3) < (10000)) {
            tick_inputs(NULL, NULL, consoleType);
            return;
        }
        last_poll_dj_ps3 = micros();
    }
#endif
#ifdef TICK_WII
    tick_wiioutput();
#endif
    bool ready = tick_usb();

    // Input queuing is enabled, tick as often as possible
    if (INPUT_QUEUE && !ready) {
        tick_inputs(NULL, NULL, consoleType);
    }

    last_poll = micros();
    // Tick the controller every 5ms if this device is usb only, is connected to a usb port, and usb is not ready
    if (!INPUT_QUEUE && !ready && usb_configured() && millis() - lastSentPacket > 5) {
        lastSentPacket = millis();
        tick_inputs(NULL, NULL, consoleType);
    }
}

void device_reset(void) {
#if USB_HOST_STACK
    if (consoleType == XBOXONE) {
        if (xbox_one_state != Announce && xbox_one_state != WaitingDesc1) {
            powerMode.subcommand = 0x07;
            USB_Device_Type_t type = get_device_address_for(XBOXONE);
            send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&powerMode, sizeof(GipPowerMode_t));
            powerMode.subcommand = 0x00;
        }
    }
#endif
    xbox_one_state = Announce;
    data_from_controller_size = 0;
    data_from_console_size = 0;
    hid_sequence_number = 1;
    report_sequence_number = 1;
}

uint8_t last_len = 0;
void receive_report_from_controller(uint8_t const *report, uint16_t len) {
    if (report[0] == GIP_INPUT_REPORT) {
        report_sequence_number = report[2] + 1;
    }
    const GipHeader_t *header = (const GipHeader_t *)report;
    if (header->command == 0x02) {
        return;
    }
    if (xbox_one_state != Auth) {
        return;
    }
    data_from_controller_size = len;
    memcpy(data_from_controller, report, len);
}

void xinput_controller_connected(uint16_t vid, uint16_t pid) {
    handle_player_leds(0);
    if (xbox_360_state == Authenticated) return;
    xbox_360_vid = vid;
    xbox_360_pid = pid;
}

void xinput_w_controller_connected() {
    handle_player_leds(0);
}

void xone_controller_connected(uint8_t dev_addr, uint8_t instance) {
    printf("Sending to controller %d\r\n", dev_addr);
    send_report_to_controller(dev_addr, instance, (uint8_t *)&powerMode, sizeof(GipPowerMode_t));
}
bool xone_controller_send_init_packet(uint8_t dev_addr, uint8_t instance, uint8_t id) {
    if (id == 0) {
        send_report_to_controller(dev_addr, instance, (uint8_t *)&ledOn, sizeof(ledOn));
        return true;
    } else if (id == 1 && consoleType != XBOXONE) {
        // When used for auth, the console will send this
        send_report_to_controller(dev_addr, instance, (uint8_t *)&authDonePacket, sizeof(authDonePacket));
        return true;
    }
    return false;
}

void host_controller_connected() {
    // With input_usb_host, we need to run detection for a little bit after the input is detected
#ifdef INPUT_USB_HOST
    input_start = millis();
#endif
}

void ps4_controller_connected(uint8_t dev_addr, uint16_t vid, uint16_t pid) {
    if (vid == SONY_VID && (pid == PS4_DS_PID_1 || pid == PS4_DS_PID_2 || pid == PS4_DS_PID_3)) {
        handle_player_leds(0);
    }
    auth_ps4_controller_found = true;
    auth_ps4_is_ghl = vid == XBOX_REDOCTANE_VID && pid == PS4_GHLIVE_DONGLE_PID;
}

void ps3_controller_connected(uint8_t dev_addr, uint16_t vid, uint16_t pid) {
    if (vid == SONY_VID && pid == SONY_DS3_PID) {
        // Enable PS3 reports
        uint8_t hid_command_enable[] = {0x42, 0x0c, 0x00, 0x00};
        transfer_with_usb_controller(dev_addr, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x03F4, 0x00, sizeof(hid_command_enable), hid_command_enable);
        handle_player_leds(0);
    }
    if (vid == REDOCTANE_VID && (pid == PS3_KEYBOARD_PID || pid == PS3_MUSTANG_PID || pid == PS3_SQUIRE_PID || pid == PS3_MPA_MUSTANG_PID || pid == PS3_MPA_SQUIRE_PID || pid == PS3_MPA_KEYBOARD_PID)) {
        uint8_t hid_command_enable[40] = {
            0xE9, 0x00, 0x89, 0x1B, 0x00, 0x00, 0x00, 0x02,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
            0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xE9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        transfer_with_usb_controller(dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0300, 0, sizeof(hid_command_enable), hid_command_enable);
    }
}

void ps4_controller_disconnected(void) {
    auth_ps4_controller_found = false;
}

void set_console_type(uint8_t new_console_type) {
    if (consoleType == new_console_type && new_console_type != UNIVERSAL) return;
    consoleType = new_console_type;
    reset_usb();
}
#if USB_HOST_STACK || BLUETOOTH_RX
void get_usb_device_type_for(uint16_t vid, uint16_t pid, uint16_t version, USB_Device_Type_t *type) {
    switch (vid) {
        case STREAM_DECK_VID: {
            type->console_type = STREAM_DECK;
            type->sub_type = UNKNOWN;
            switch (pid) {
                case STREAM_DECK_OG_PID:
                    type->sub_type = STREAM_DECK_OG;
                    break;
                case STREAM_DECK_MINI_PID:
                    type->sub_type = STREAM_DECK_MINI;
                    break;
                case STREAM_DECK_XL_PID:
                    type->sub_type = STREAM_DECK_XL;
                    break;
                case STREAM_DECK_V2_PID:
                    type->sub_type = STREAM_DECK_V2;
                    break;
                case STREAM_DECK_MK2_PID:
                    type->sub_type = STREAM_DECK_MK2;
                    break;
                case STREAM_DECK_PLUS_PID:
                    type->sub_type = STREAM_DECK_PLUS;
                    break;
                case STREAM_DECK_PEDAL_PID:
                    type->sub_type = STREAM_DECK_PEDAL;
                    break;
                case STREAM_DECK_XLV2_PID:
                    type->sub_type = STREAM_DECK_XLV2;
                    break;
                case STREAM_DECK_MINIV2_PID:
                    type->sub_type = STREAM_DECK_MINIV2;
                    break;
                case STREAM_DECK_NEO_PID:
                    type->sub_type = STREAM_DECK_NEO;
                    break;
            }
            break;
        }
        case NINTENDO_VID: {
            if (pid == SWITCH_PRO_PID) {
                type->console_type = SWITCH;
                type->sub_type = GAMEPAD;
            }
            break;
        }
        case ARDWIINO_VID: {
            if (pid == ARDWIINO_PID) {
                type->console_type = SANTROLLER;
                type->sub_type = (version >> 8) & 0xFF;
            }
            break;
        }
        case RAPHNET_VID: {
            type->console_type = RAPHNET;
            type->sub_type = GAMEPAD;
            break;
        }
        case MAGICBOOTS_PS4_VID: {
            if (pid == MAGICBOOTS_PS4_PID) {
                type->console_type = PS4;
                type->sub_type = GAMEPAD;
            }
            break;
        }
        case ARDUINO_VID: {
            if (pid == STEPMANIA_X_PID) {
                type->console_type = STEPMANIAX;
                type->sub_type = DANCE_PAD;
            }
            break;
        }
        case LTEK_LUFA_VID: {
            if (pid == LTEK_LUFA_PID) {
                type->console_type = LTEK_ID;
                type->sub_type = DANCE_PAD;
            }
            break;
        }
        case LTEK_VID: {
            if (pid == LTEK_PID) {
                type->console_type = LTEK;
                type->sub_type = DANCE_PAD;
            }
            break;
        }
        case SONY_VID:
            switch (pid) {
                case SONY_DS3_PID:
                    type->console_type = PS3;
                    type->sub_type = GAMEPAD;
                    break;
                case PS4_DS_PID_1:
                case PS4_DS_PID_2:
                case PS4_DS_PID_3:
                    type->console_type = PS4;
                    type->sub_type = GAMEPAD;
                    break;
                case PS5_DS_PID:
                    type->console_type = PS5;
                    type->sub_type = GAMEPAD;
                    break;
            }
            break;
        case REDOCTANE_VID:
            switch (pid) {
                case PS3_GH_GUITAR_PID:
                    type->console_type = PS3;
                    type->sub_type = GUITAR_HERO_GUITAR;
                    break;
                case PS3_GH_DRUM_PID:
                    type->console_type = PS3;
                    type->sub_type = GUITAR_HERO_DRUMS;
                    break;
                case PS3_RB_GUITAR_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case PS3_RB_DRUM_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_DRUMS;
                    break;
                case PS3_DJ_TURNTABLE_PID:
                    type->console_type = PS3;
                    type->sub_type = DJ_HERO_TURNTABLE;
                    break;
                case PS3WIIU_GHLIVE_DONGLE_PID:
                    type->console_type = PS3;
                    type->sub_type = LIVE_GUITAR;
                    break;
                case PS3_MPA_KEYBOARD_PID:
                case PS3_KEYBOARD_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_KEYS;
                    break;
                case PS3_MUSTANG_PID:
                case PS3_MPA_MUSTANG_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_MUSTANG;
                    break;
                case PS3_SQUIRE_PID:
                case PS3_MPA_SQUIRE_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_SQUIRE;
            }
            break;

        case HARMONIX_VID:
            // Polled the same as PS3, so treat them as PS3 instruments
            switch (pid) {
                case WII_RB_GUITAR_PID:
                case WII_RB_GUITAR_2_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;

                case WII_RB_DRUM_PID:
                case WII_RB_DRUM_2_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_DRUMS;
                    break;
                case WII_KEYBOARD_PID:
                case WII_MPA_KEYBOARD_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_KEYS;
                    break;
                case WII_MUSTANG_PID:
                case WII_MPA_MUSTANG_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_MUSTANG;
                case WII_SQUIRE_PID:
                case WII_MPA_SQUIRE_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_SQUIRE;
                    break;
            }

            break;
        case PDP_VID:
            switch (pid) {
                case XBOX_ONE_JAG_PID:
                case XBOX_ONE_RIFFMASTER_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case PS4_JAG_PID:
                case PS4_RIFFMASTER_PID:
                    type->console_type = PS4;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case PS5_RIFFMASTER_PID:
                    type->console_type = PS5;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
            }

            break;

        case MAD_CATZ_VID:
            switch (pid) {
                case XBOX_ONE_RB_GUITAR_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case XBOX_ONE_RB_DRUM_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = ROCK_BAND_DRUMS;
                    break;
                case PS4_STRAT_PID:
                    type->console_type = PS4;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
            }

            break;

        case XBOX_REDOCTANE_VID:
            switch (pid) {
                case XBOX_ONE_GHLIVE_DONGLE_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = LIVE_GUITAR;
                    break;
                case XBOX_360_GHLIVE_DONGLE_PID:
                    type->console_type = XBOX360;
                    type->sub_type = XINPUT_GUITAR_HERO_LIVE;
                    break;
                case XBOX_360_WT_KIOSK_PID:
                    type->console_type = XBOX360;
                    type->sub_type = XINPUT_GUITAR_WT;
                    break;
            }
            break;
    }
}
#endif
