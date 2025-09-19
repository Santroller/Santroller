#include "shared_main.h"

#include "Arduino.h"
#include "accel.h"
#include "bh_drum.h"
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
#include "state_translation/drums.h"
#include "state_translation/pro_guitar.h"
#include "state_translation/pro_keys.h"
#include "usbhid.h"
#include "util.h"
#include "wii.h"
#include "wt_drum.h"
#define UP 1 << 0
#define DOWN 1 << 1
#define LEFT 1 << 2
#define RIGHT 1 << 3
static const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
static const uint8_t dpad_bindings_reverse[] = {UP, UP | RIGHT, RIGHT, DOWN | RIGHT, DOWN, DOWN | LEFT, LEFT, UP | LEFT};
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
#define GH5NECK_ADDR 0x0D
#define CLONE_ADDR 0x10
#define CLONE_VALID_PACKET 0x52
#define GH5NECK_BUTTONS_PTR 0x12
#define BUFFER_SIZE_QUEUE 255
#define KEY_ERR_OVF 0x01
#define REQUIRE_LED_DEBOUNCE LED_COUNT || LED_COUNT_PERIPHERAL || LED_COUNT_STP || LED_COUNT_PERIPHERAL_STP || LED_COUNT_WS2812 || LED_COUNT_PERIPHERAL_WS2812 || HAS_LED_OUTPUT || LED_COUNT_MPR121
struct
{
    union
    {
        signed int ltt : 6;
        struct
        {
            unsigned int ltt40 : 5;
            unsigned int ltt5 : 1;
        };
    };
} ltt_t;
struct
{
    union
    {
        signed int rtt : 6;
        struct
        {
            unsigned int rtt0 : 1;
            unsigned int rtt21 : 2;
            unsigned int rtt43 : 2;
            unsigned int rtt5 : 1;
        };
    };
} rtt_t;
crkd_neck_t lastCrkd;
unsigned long lastMidi = 0;
Midi_Data_t midiData = {0};
void onNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    // velocities are 7 bit
    // printf("Note ON ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    velocity = velocity << 1;
#if DEVICE_TYPE_IS_DRUM
    // drums, base inputs on triggers
    if (velocity > midiData.midiVelocitiesTemp[note])
    {
        midiData.midiVelocities[note] = velocity;
        midiData.midiVelocitiesTemp[note] = velocity;
    }
#else
    // Handle midi normally
    midiData.midiVelocities[note] = velocity;
    midiData.midiVelocitiesTemp[note] = velocity;
#endif
    lastMidi = millis();
}

void offNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    // printf("Note OFF ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    // ignore note off for drums as we handle that ourselves, and not all drums send it
#if !(DEVICE_TYPE_IS_DRUM)
    midiData.midiVelocities[note] = 0;
    midiData.midiVelocitiesTemp[note] = 0;
#endif
}

void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2)
{
    // cc are 7 bit
    printf("ControlChange ch=%d, b1=%d, b2=%d\r\n", channel, b1, b2);
    if (b1 == MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL)
    {
        midiData.midiSustainPedal = b2 << 1;
    }
    if (b1 == MIDI_CONTROL_COMMAND_MOD_WHEEL)
    {
        midiData.midiModWheel = b2 << 1;
    }
}

void onPitchBend(uint8_t channel, int pitch)
{
    // pitchbend is signed 14 bit
    printf("PitchBend ch=%d, pitch=%d\r\n", channel, pitch);
    midiData.midiPitchWheel = pitch << 2;
}
PS3RockBandProGuitar_Data_t sysexGuitar = {0};
void onSysEx(uint8_t *data, unsigned size)
{
#if DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_MUSTANG || DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_SQUIRE
    uint8_t type, fret, velocity;
    type = data[5];
    uint8_t string = (size >= 5) ? (data[6] - 1) % 6 : 0;
    // changes to the fret state
    if ((type == 0x01) && (size >= 6))
    {
        fret = data[7];
        // offset fret numbers relative to the base note of each string
        switch (string)
        {
        case 0:
            sysexGuitar.lowEFret = fret - 0x40;
            break;
        case 1:
            sysexGuitar.aFret = fret - 0x3B;
            break;
        case 2:
            sysexGuitar.dFret = fret - 0x37;
            break;
        case 3:
            sysexGuitar.gFret = fret - 0x32;
            break;
        case 4:
            sysexGuitar.bFret = fret - 0x2D;
            break;
        case 5:
            sysexGuitar.highEFret = fret - 0x28;
            break;
        }
    }
    // picking events
    else if ((type == 0x05) && (size >= 6))
    {
        velocity = data[7];
        switch (string)
        {
        case 0:
            sysexGuitar.lowEFretVelocity = velocity;
            break;
        case 1:
            sysexGuitar.aFretVelocity = velocity;
            break;
        case 2:
            sysexGuitar.dFretVelocity = velocity;
            break;
        case 3:
            sysexGuitar.gFretVelocity = velocity;
            break;
        case 4:
            sysexGuitar.bFretVelocity = velocity;
            break;
        case 5:
            sysexGuitar.highEFretVelocity = velocity;
            break;
        }
    }
    // button events
    else if ((type == 0x08) && (size >= 7))
    {
        // PS3 style report starts at byte 4
        memcpy(&sysexGuitar, data + 6, 3);
        // tilt however is not in the same spot as a ps3 report
        sysexGuitar.tilt = (data[8] & 0x40) ? 0x7f : 0x40;
    }
#endif
}
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
#ifdef INPUT_DJ_TURNTABLE_SMOOTHING
int16_t dj_sum_left = 0;
int8_t dj_last_readings_left[TURNTABLE_BUFFER_SIZE];
int8_t dj_next_left = 0;
int16_t dj_sum_right = 0;
int8_t dj_last_readings_right[TURNTABLE_BUFFER_SIZE];
int8_t dj_next_right = 0;
#endif
USB_Report_Data_t combined_report;
#if DEVICE_TYPE_IS_GAMEPAD
PS3_REPORT bt_report;
#else
USB_Report_Data_t bt_report;
#endif
// Note for future explorers: it seems like the debounce array is always used for digital inputs,
// regardless of whether debouncing is actually enabled. Idle timeout/shutdown stuff
// relies on this, although it's unconfirmed without access to configurator source code.
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
long lastInputActivity = 0; // used for inactivity timeout(s)
long lastHeartbeat = 0;
long lastInactivityPulse = 0;
bool startedInactivityPulse = false; // used to ignore inputs while outputting pulse
                                     // (for hardware wired with input and output via same pin)
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
bool lastCrkdWasSuccessful = false;
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
bool report_requested = false;
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
#if LED_COUNT_WS2812
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
#if LED_COUNT_PERIPHERAL_WS2812
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

uint8_t gh5_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};
void setKey(uint8_t id, uint8_t key, USB_6KRO_Data_t *report)
{
    for (size_t i = 0; i < 6; i++)
    {
        if (report->KeyCode[i] == key)
        {
            return;
        }
        if (!report->KeyCode[i])
        {
            report->KeyCode[i] = key;
            return;
        }
    }
    // Too many buttons - flag the overflow condition and clear all key indexes
    memset(report->KeyCode, KEY_ERR_OVF, sizeof(report->KeyCode));
    return;
}
uint8_t seq = 1;
void init_main(void)
{
#if !DEVICE_TYPE_IS_GAMEPAD
    consoleType = UNIVERSAL;
#endif
    initPins();
    twi_init();
    spi_begin();
    uart_begin();
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
#ifdef INPUT_DJ_TURNTABLE_SMOOTHING
    memset(dj_last_readings_left, 0, sizeof(dj_last_readings_left));
    memset(dj_last_readings_right, 0, sizeof(dj_last_readings_right));
#endif
#ifdef INPUT_PS2
    init_ack();
#endif
#ifdef TICK_PS2
    ps2_emu_init();
#endif
    memset(midiData.midiVelocities, 0, sizeof(midiData.midiVelocities));
#ifdef TICK_WII
    initWiiOutput();
#endif
}
#ifdef SLAVE_TWI_PORT
bool slave_initted = false;
void tick_slave()
{
    if (slave_initted)
    {
        return;
    }
    if (!slaveInit())
    {
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
int16_t adc_i(uint8_t pin)
{
    int32_t ret = adc(pin);
    return ret - 32767;
}
int16_t handle_calibration_xbox(int16_t previous, int32_t orig_val, int16_t min, int16_t max, int16_t center, int16_t deadzone)
{
    int32_t val = orig_val;
    if (val > INT16_MAX)
    {
        val = INT16_MAX;
    }
    if (val < INT16_MIN)
    {
        val = INT16_MIN;
    }
    if (val < center)
    {
        if ((center - val) < deadzone)
        {
            val = 0;
        }
        else
        {
            val = map(val, min, center - deadzone, INT16_MIN, 0);
        }
    }
    else
    {
        if ((val - center) < deadzone)
        {
            val = 0;
        }
        else
        {
            val = map(val, center + deadzone, max, 0, INT16_MAX);
        }
    }
    if (val > INT16_MAX)
    {
        val = INT16_MAX;
    }
    if (val < INT16_MIN)
    {
        val = INT16_MIN;
    }
    if (abs(val) > abs(previous))
    {
        return val;
    }
    return previous;
}

int16_t handle_calibration_xbox_whammy(int16_t previous, uint32_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone)
{
    int32_t val = orig_val;
    if (multiplier > 0)
    {
        if ((val - min) < deadzone)
        {
            return INT16_MIN;
        }
    }
    else
    {
        if (val > min)
        {
            return INT16_MIN;
        }
    }
    val -= min;
    val *= multiplier;
    val /= 512;
    val += INT16_MIN;
    if (val > INT16_MAX)
    {
        val = INT16_MAX;
    }
    if (val < INT16_MIN)
    {
        val = INT16_MIN;
    }
    if (val > previous)
    {
        return val;
    }
    return previous;
}
uint16_t handle_calibration_xbox_one_trigger(uint16_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone)
{
    int32_t val = orig_val;
    if (multiplier > 0)
    {
        if ((val - min) < deadzone)
        {
            return 0;
        }
    }
    else
    {
        if (val > min)
        {
            return 0;
        }
    }
    val -= min;
    val *= multiplier;
    val /= 512;
    if (val > UINT16_MAX)
    {
        val = UINT16_MAX;
    }
    if (val < 0)
    {
        val = 0;
    }
    val = val >> 6;
    if (val > previous)
    {
        return val;
    }
    return previous;
}
uint16_t handle_calibration_drum(uint16_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone)
{
    int32_t val = orig_val;
    if (multiplier > 0)
    {
        if ((val - min) < deadzone)
        {
            return 0;
        }
    }
    else
    {
        if (val > min)
        {
            return 0;
        }
    }
    val -= min;
    val *= multiplier;
    val /= 512;
    if (val > UINT16_MAX)
    {
        val = UINT16_MAX;
    }
    if (val < 0)
    {
        val = 0;
    }
    if (val > previous)
    {
        return val;
    }
    return previous;
}
uint8_t handle_calibration_ps3(uint8_t previous, int32_t orig_val, int16_t min, int16_t max, int16_t center, int16_t deadzone)
{
    int8_t ret = handle_calibration_xbox((previous - PS3_STICK_CENTER) << 8, orig_val, min, max, center, deadzone) >> 8;
    return (uint8_t)(ret + PS3_STICK_CENTER);
}

int8_t handle_calibration_mouse(int8_t previous, int32_t orig_val, int16_t min, int16_t max, int16_t center, int16_t deadzone)
{
    return handle_calibration_xbox(previous << 8, orig_val, min, max, center, deadzone) >> 8;
}

uint8_t handle_calibration_ps3_360_trigger(uint8_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone)
{
    return handle_calibration_xbox_one_trigger(previous << 2, orig_val, min, multiplier, deadzone) >> 2;
}

uint16_t handle_calibration_ps3_accel(uint16_t previous, int32_t orig_val, int16_t min, int16_t max, int16_t center, uint16_t deadzone)
{
#if DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE_IS_LIVE_GUITAR
    // For whatever reason, the acceleration for guitars swings between -128 to 128, not -512 to 512
    // Also, the game is looking for the value going negative, not positive
    int16_t ret = (-(handle_calibration_xbox(-((previous - PS3_ACCEL_CENTER) << 8), orig_val, min, max, center, deadzone) >> 8));
#else
    int16_t ret = handle_calibration_xbox((previous - PS3_ACCEL_CENTER) << 6, orig_val, min, max, center, deadzone) >> 6;
#endif
    return PS3_ACCEL_CENTER + ret;
}
uint8_t handle_calibration_arcade_accel(uint16_t previous, int32_t orig_val, int16_t min, int16_t max, int16_t center, uint16_t deadzone)
{
    int8_t ret = handle_calibration_xbox((previous - 65) << 9, orig_val, min, max, center, deadzone) >> 9;
    return (uint8_t)(ret + 65);
}
long last_zero = 0;
uint8_t handle_calibration_ps3_whammy(uint8_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone)
{
#if DEVICE_TYPE == ROCK_BAND_GUITAR
    // RB whammy is full range
    return handle_calibration_ps3_360_trigger(previous, orig_val, min, multiplier, deadzone);
#else
    // GH whammy ignores the negative half of the axis, so shift to get between 0 and 127, then add center
    uint8_t ret = handle_calibration_ps3_360_trigger((previous - PS3_STICK_CENTER) << 1, orig_val, min, multiplier, deadzone) >> 1;
    return ret + PS3_STICK_CENTER;
#endif
}
uint8_t handle_calibration_ps2_whammy(uint8_t previous, uint16_t orig_val, uint32_t min, int16_t multiplier, uint16_t deadzone)
{
    // GH whammy ignores the negative half of the axis, so shift to get between 0 and 127, then subtract from center
    uint8_t ret = handle_calibration_ps3_360_trigger((0x7f - previous) << 1, orig_val, min, multiplier, deadzone) >> 1;
    return 0x7f - ret;
}

uint8_t handle_calibration_turntable_ps3(uint8_t previous, int16_t orig_val, uint8_t multiplier)
{
    int32_t val = (orig_val * multiplier) >> 8;
    if (val > INT8_MAX)
    {
        val = INT8_MAX;
    }
    if (val < INT8_MIN)
    {
        val = INT8_MIN;
    }
    // previous is uint, convert to int to test
    if (abs(val) > abs((int)previous - PS3_STICK_CENTER))
    {
        // ps3 wants uint, convert to uint
        return (uint8_t)(val + PS3_STICK_CENTER);
    }
    return previous;
}
int16_t handle_calibration_turntable_360(int16_t previous, int16_t orig_val, uint8_t multiplier)
{
    if (consoleType == WINDOWS && WINDOWS_TURNTABLE_FULLRANGE)
    {
        int32_t val = (orig_val * multiplier);
        if (val > INT16_MAX)
        {
            val = INT16_MAX;
        }
        if (val < INT16_MIN)
        {
            val = INT16_MIN;
        }
        if (abs(val) > abs(previous))
        {
            return (int16_t)val;
        }
        return previous;
    }
    int32_t val = (orig_val * multiplier) >> 8;
    if (val > 128)
    {
        val = 128;
    }
    if (val < -128)
    {
        val = -128;
    }
    if (abs(val) > abs(previous))
    {
        return (int16_t)val;
    }
    return previous;
}
uint16_t descriptor_index = 0;
long reset_after_360 = 0;
uint8_t tick_xbox_one()
{
    uint32_t serial = micros();
    switch (xbox_one_state)
    {
    case Announce:
        descriptor_index = 0;
        xbox_one_state = WaitingDesc1;
        memcpy(&combined_report, announce, sizeof(announce));
        memcpy(&combined_report.raw[7], &serial, 3);
        return sizeof(announce);
    case IdentDesc1:
        xbox_one_state = WaitingDesc;
        descriptor_index += 64;
        memcpy(&combined_report, xb1_descriptor, 64);
        return 64;
    case IdentDesc:
    {
        uint16_t len = sizeof(xb1_descriptor) - descriptor_index;
        if (len > 64)
        {
            len = 64;
        }
        else
        {
            xbox_one_state = WaitingDescEnd;
        }
        memcpy(&combined_report, xb1_descriptor + descriptor_index, len);
        descriptor_index += len;
        return len;
    }
    case IdentDescEnd:
    {
        xbox_one_state = Auth;
        memcpy(&combined_report, xb1_descriptor_end, sizeof(xb1_descriptor_end));
        return sizeof(xb1_descriptor_end);
    }
    case Auth:
        if (data_from_controller_size)
        {
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
static uint8_t xinput_rb_velocity_positive(int16_t velocity)
{
    return (uint8_t)(255 - (uint8_t)(velocity >> 7));
}
static uint8_t xinput_rb_velocity_negative(int16_t velocity)
{
    if (velocity == 0)
        return 255;

    return (uint8_t)(255 - (uint8_t)((~velocity) >> 7));
}
long lastTick;
uint8_t keyboard_report = 0;
void convert_report(const uint8_t *data, uint8_t len, USB_Device_Type_t device_type, USB_Host_Data_t *usb_host_data)
{
    switch (device_type.console_type)
    {
    case STADIA:
    {
        Stadia_Data_t *report = (Stadia_Data_t *)data;
        uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
        asm volatile("" ::
                         : "memory");
        bool up = dpad & UP;
        bool left = dpad & LEFT;
        bool down = dpad & DOWN;
        bool right = dpad & RIGHT;
        usb_host_data->a |= report->a;
        usb_host_data->b |= report->b;
        usb_host_data->x |= report->x;
        usb_host_data->y |= report->y;
        usb_host_data->capture |= report->capture;
        usb_host_data->leftShoulder |= report->leftShoulder;
        usb_host_data->rightShoulder |= report->rightShoulder;
        usb_host_data->back |= report->back;
        usb_host_data->start |= report->start;
        usb_host_data->guide |= report->guide;
        usb_host_data->leftThumbClick |= report->leftThumbClick;
        usb_host_data->rightThumbClick |= report->rightThumbClick;
        usb_host_data->dpadLeft |= left;
        usb_host_data->dpadRight |= right;
        usb_host_data->dpadUp |= up;
        usb_host_data->dpadDown |= down;
        if (report->leftTrigger)
        {
            usb_host_data->leftTrigger = report->leftTrigger << 8;
        }
        if (report->rightTrigger)
        {
            usb_host_data->rightTrigger = report->rightTrigger << 8;
        }
        if (report->leftStickX != PS3_STICK_CENTER)
        {
            usb_host_data->leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
        }
        if (report->leftStickY != PS3_STICK_CENTER)
        {
            usb_host_data->leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
        }
        if (report->rightStickX != PS3_STICK_CENTER)
        {
            usb_host_data->rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
        }
        if (report->rightStickY != PS3_STICK_CENTER)
        {
            usb_host_data->rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
        }
    }
    case STREAM_DECK:
    {
        uint8_t offset = 1;
        switch (device_type.sub_type)
        {
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
        for (uint8_t i = 0; i + offset < len && i < 16; i++)
        {
            bit_write(data[i + offset], usb_host_data->genericButtons, i);
        }
        break;
    }
    case KEYBOARD:
    {
        USB_6KRO_Boot_Data_t *report = (USB_6KRO_Boot_Data_t *)data;
        usb_host_data->keyboard.leftCtrl = report->leftCtrl;
        usb_host_data->keyboard.leftShift = report->leftShift;
        usb_host_data->keyboard.leftAlt = report->leftAlt;
        usb_host_data->keyboard.lWin = report->lWin;
        usb_host_data->keyboard.rightCtrl = report->rightCtrl;
        usb_host_data->keyboard.rightShift = report->rightShift;
        usb_host_data->keyboard.rightAlt = report->rightAlt;
        usb_host_data->keyboard.rWin = report->rWin;
        uint8_t *keyData = usb_host_data->keyboard.raw;
        for (uint8_t i = 0; i < SIMULTANEOUS_KEYS; i++)
        {
            uint8_t keycode = report->KeyCode[i];
            // F24 is the last supported key in our nkro report
            if (keycode && keycode <= KEYCODE_F24)
            {
                bit_set(keyData[keycode >> 3], keycode & 7);
            }
        }
        break;
    }
    case MOUSE:
    {
        USB_Mouse_Boot_Data_t *report = (USB_Mouse_Boot_Data_t *)data;
        memcpy(&usb_host_data->mouse, report, sizeof(USB_Mouse_Boot_Data_t));
        break;
    }
    case GENERIC:
    {
        USB_Host_Data_t *report = (USB_Host_Data_t *)data;
        usb_host_data->genericAxisX = report->genericAxisX;
        usb_host_data->genericAxisY = report->genericAxisY;
        usb_host_data->genericAxisZ = report->genericAxisZ;
        usb_host_data->genericAxisRx = report->genericAxisRx;
        usb_host_data->genericAxisRy = report->genericAxisRy;
        usb_host_data->genericAxisRz = report->genericAxisRz;
        usb_host_data->genericAxisSlider = report->genericAxisSlider;
        usb_host_data->dpadLeft |= report->dpadLeft;
        usb_host_data->dpadRight |= report->dpadRight;
        usb_host_data->dpadUp |= report->dpadUp;
        usb_host_data->dpadDown |= report->dpadDown;
        usb_host_data->genericButtons |= report->genericButtons;
        break;
    }
    case RAPHNET:
    {
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            RaphnetGamepad_Data_t *report = (RaphnetGamepad_Data_t *)data;
            usb_host_data->leftStickX = report->leftJoyX - 16000;
            usb_host_data->leftStickY = report->leftJoyY - 16000;
            usb_host_data->rightStickX = report->rightJoyX - 16000;
            usb_host_data->rightStickY = report->rightJoyY - 16000;
            usb_host_data->leftTrigger = report->leftTrigger;
            usb_host_data->rightTrigger = report->rightTrigger;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->start |= report->start;
            usb_host_data->back |= report->select;
            usb_host_data->guide |= report->home;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->dpadLeft |= report->left;
            usb_host_data->dpadRight |= report->right;
            usb_host_data->dpadUp |= report->up;
            usb_host_data->dpadDown |= report->down;
            break;
        }
        case GUITAR_HERO_GUITAR:
        {
            RaphnetGuitar_Data_t *report = (RaphnetGuitar_Data_t *)data;
            usb_host_data->leftStickX = report->joyX - 16000;
            usb_host_data->leftStickY = report->joyY - 16000;
            usb_host_data->whammy = report->whammy >> 8;
            usb_host_data->start = report->plus;
            usb_host_data->back = report->minus;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->a |= report->green;
            usb_host_data->b |= report->red;
            usb_host_data->y |= report->yellow;
            usb_host_data->x |= report->blue;
            usb_host_data->leftShoulder |= report->orange;
            usb_host_data->dpadUp |= report->up;
            usb_host_data->dpadDown |= report->down;
            break;
        }
        case GUITAR_HERO_DRUMS:
        {
            RaphnetDrum_Data_t *report = (RaphnetDrum_Data_t *)data;
            usb_host_data->leftStickX = report->joyX - 16000;
            usb_host_data->leftStickY = report->joyY - 16000;
            usb_host_data->start = report->plus;
            usb_host_data->back = report->minus;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            // TODO: figure out kick mappings for raphnet
            // usb_host_data->kick1 |= report->kick1;
            usb_host_data->a |= report->green;
            usb_host_data->b |= report->red;
            usb_host_data->y |= report->yellow;
            usb_host_data->x |= report->blue;
            usb_host_data->leftShoulder |= report->orange;
            break;
        }
        }
        break;
    }
    case SANTROLLER:
    {
        PCGamepad_Data_t *report = (PCGamepad_Data_t *)data;
        uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
        asm volatile("" ::
                         : "memory");
        bool up = dpad & UP;
        bool left = dpad & LEFT;
        bool down = dpad & DOWN;
        bool right = dpad & RIGHT;
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        case DANCE_PAD:
        case STAGE_KIT:
        {
            PCGamepad_Data_t *report = (PCGamepad_Data_t *)data;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->leftStickY != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
            }
            if (report->rightStickX != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->rightStickY != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
            }
            break;
        }
        case GUITAR_HERO_GUITAR:
        {
            PCGuitarHeroGuitar_Data_t *report = (PCGuitarHeroGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            if (report->tilt != PS3_STICK_CENTER)
            {
                usb_host_data->tilt = (report->tilt - PS3_STICK_CENTER) << 8;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            usb_host_data->slider = report->slider;
            break;
        }
        case ROCK_BAND_GUITAR:
        {
            PCRockBandGuitar_Data_t *report = (PCRockBandGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            if (report->tilt != PS3_STICK_CENTER)
            {
                usb_host_data->tilt = (report->tilt - PS3_STICK_CENTER) << 8;
            }
            usb_host_data->soloGreen |= report->soloGreen;
            usb_host_data->soloRed |= report->soloRed;
            usb_host_data->soloYellow |= report->soloYellow;
            usb_host_data->soloBlue |= report->soloBlue;
            usb_host_data->soloOrange |= report->soloOrange;

            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            if (report->pickup)
            {
                usb_host_data->pickup = report->pickup;
            }
            break;
        }
        case GUITAR_HERO_DRUMS:
        {
            PCGuitarHeroDrums_Data_t *report = (PCGuitarHeroDrums_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;

            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->rightShoulder;
            SET_GH_PADS();
            break;
        }
        case ROCK_BAND_DRUMS:
        {
            PCRockBandDrums_Data_t *report = (PCRockBandDrums_Data_t *)data;
            if (!report->padFlag && !report->cymbalFlag)
            {
                usb_host_data->a |= report->a;
                usb_host_data->b |= report->b;
                usb_host_data->x |= report->x;
                usb_host_data->y |= report->y;
                usb_host_data->dpadLeft |= left;
                usb_host_data->dpadRight |= right;
                usb_host_data->dpadUp |= up;
                usb_host_data->dpadDown |= down;
            }
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            uint8_t redVelocity = report->redVelocity;
            uint8_t greenVelocity = report->greenVelocity;
            uint8_t yellowVelocity = report->yellowVelocity;
            uint8_t blueVelocity = report->blueVelocity;
            bool kick1 = report->leftShoulder;
            bool kick2 = report->rightShoulder;
            SET_RB_PADS();
            break;
        }
        case LIVE_GUITAR:
        {
            PCGHLGuitar_Data_t *report = (PCGHLGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            if (report->tilt != PS3_STICK_CENTER)
            {
                usb_host_data->tilt = (report->tilt - PS3_STICK_CENTER) << 8;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            break;
        }
        case DJ_HERO_TURNTABLE:
        {
            PCTurntable_Data_t *report = (PCTurntable_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftBlue |= report->leftBlue;
            usb_host_data->leftRed |= report->leftRed;
            usb_host_data->leftGreen |= report->leftGreen;
            usb_host_data->rightBlue |= report->rightBlue;
            usb_host_data->rightRed |= report->rightRed;
            usb_host_data->rightGreen |= report->rightGreen;
            if (report->effectsKnob != PS3_ACCEL_CENTER)
            {
                usb_host_data->effectsKnob = (report->effectsKnob - PS3_ACCEL_CENTER) << 6;
            }
            if (report->crossfader != PS3_ACCEL_CENTER)
            {
                usb_host_data->crossfader = (report->crossfader - PS3_ACCEL_CENTER) << 6;
            }
            if (report->leftTableVelocity != PS3_STICK_CENTER)
            {
                usb_host_data->leftTableVelocity = (report->leftTableVelocity - PS3_STICK_CENTER) << 8;
            }
            if (report->rightTableVelocity != PS3_STICK_CENTER)
            {
                usb_host_data->rightTableVelocity = (report->rightTableVelocity - PS3_STICK_CENTER) << 8;
            }
            break;
        }
        case ROCK_BAND_PRO_KEYS:
        {
            PCRockBandProKeyboard_Data_t *report = (PCRockBandProKeyboard_Data_t *)data;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            TRANSLATE_PRO_KEYS;
            break;
        }
        case ROCK_BAND_PRO_GUITAR_SQUIRE:
        case ROCK_BAND_PRO_GUITAR_MUSTANG:
        {
            PCRockBandProGuitar_Data_t *report = (PCRockBandProGuitar_Data_t *)data;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            TRANSLATE_PRO_GUITAR;
            break;
        }
        }
        break;
    }
    case PS3:
    {
        PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)data;
        uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
        asm volatile("" ::
                         : "memory");
        bool up = dpad & UP;
        bool left = dpad & LEFT;
        bool down = dpad & DOWN;
        bool right = dpad & RIGHT;
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)data;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->leftStickY != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
            }
            if (report->rightStickX != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->rightStickY != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
            }
            if (report->pressureDpadUp)
            {
                usb_host_data->pressureDpadUp = report->pressureDpadUp;
            }
            if (report->pressureDpadRight)
            {
                usb_host_data->pressureDpadRight = report->pressureDpadRight;
            }
            if (report->pressureDpadDown)
            {
                usb_host_data->pressureDpadDown = report->pressureDpadDown;
            }
            if (report->pressureDpadLeft)
            {
                usb_host_data->pressureDpadLeft = report->pressureDpadLeft;
            }
            if (report->pressureL1)
            {
                usb_host_data->pressureL1 = report->pressureL1;
            }
            if (report->pressureR1)
            {
                usb_host_data->pressureR1 = report->pressureR1;
            }
            if (report->pressureTriangle)
            {
                usb_host_data->pressureTriangle = report->pressureTriangle;
            }
            if (report->pressureCircle)
            {
                usb_host_data->pressureCircle = report->pressureCircle;
            }
            if (report->pressureCross)
            {
                usb_host_data->pressureCross = report->pressureCross;
            }
            if (report->pressureSquare)
            {
                usb_host_data->pressureSquare = report->pressureSquare;
            }
            break;
        }

        case ROCK_BAND_PRO_KEYS:
        {
            PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)data;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            TRANSLATE_PRO_KEYS;
            break;
        }
        case ROCK_BAND_GUITAR:
        {
            PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            if (report->tilt)
            {
                usb_host_data->tilt = INT16_MAX;
            }
            if (report->solo)
            {
                usb_host_data->soloGreen |= report->a;
                usb_host_data->soloRed |= report->b;
                usb_host_data->soloYellow |= report->y;
                usb_host_data->soloBlue |= report->x;
                usb_host_data->soloOrange |= report->leftShoulder;
            }
            // whammy + pickup resets to a neutral state when not in use
            // so we opt to store the last known state and use that when in neutral
            if (device_type.whammy)
            {
                usb_host_data->whammy = device_type.whammy;
            }
            if (device_type.pickup)
            {
                usb_host_data->pickup = device_type.pickup;
            }
            break;
        }
        case GUITAR_HERO_GUITAR_WT:
        case GUITAR_HERO_GUITAR:
        {
            PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            if (report->tilt != PS3_ACCEL_CENTER)
            {
                usb_host_data->tilt = (report->tilt - PS3_ACCEL_CENTER) << 8;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = (report->whammy - PS3_STICK_CENTER) << 1;
            }
            // Detect GH5 vs WT. Wait for a neutral input, then use that to detect instrument type
            if (device_type.sub_type == GUITAR_HERO_GUITAR_WT)
            {
                // Its WT, convert to GH5
                if (report->slider <= 0x2F)
                {
                    usb_host_data->slider = 0x15;
                }
                else if (report->slider <= 0x3F)
                {
                    usb_host_data->slider = 0x30;
                }
                else if (report->slider <= 0x5F)
                {
                    usb_host_data->slider = 0x4D;
                }
                else if (report->slider <= 0x6F)
                {
                    usb_host_data->slider = 0x66;
                }
                else if (report->slider <= 0x8F)
                {
                    usb_host_data->slider = 0x80;
                }
                else if (report->slider <= 0x9F)
                {
                    usb_host_data->slider = 0x9A;
                }
                else if (report->slider <= 0xAF)
                {
                    usb_host_data->slider = 0xAF;
                }
                else if (report->slider <= 0xCF)
                {
                    usb_host_data->slider = 0xC9;
                }
                else if (report->slider <= 0xEF)
                {
                    usb_host_data->slider = 0xE6;
                }
                else
                {
                    usb_host_data->slider = 0x7F;
                }
            }
            else
            {
                usb_host_data->slider = report->slider;
            }
            break;
        }
        case ROCK_BAND_DRUMS:
        {
            PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)data;
            uint8_t redVelocity = ~report->redVelocity;
            uint8_t greenVelocity = ~report->greenVelocity;
            uint8_t yellowVelocity = ~report->yellowVelocity;
            uint8_t blueVelocity = ~report->blueVelocity;
            bool kick1 = report->leftShoulder;
            bool kick2 = report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            if (!report->padFlag && !report->cymbalFlag)
            {
                usb_host_data->a |= report->a;
                usb_host_data->b |= report->b;
                usb_host_data->x |= report->x;
                usb_host_data->y |= report->y;
                usb_host_data->dpadLeft |= left;
                usb_host_data->dpadRight |= right;
                usb_host_data->dpadUp |= up;
                usb_host_data->dpadDown |= down;
            }
            SET_RB_PADS();
            break;
        }
        case GUITAR_HERO_DRUMS:
        {
            PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            SET_GH_PADS();
            // Forward any midi data we get
            if ((report->midiByte0 & 0xF0) == 0x90)
            {
                onNote(report->midiByte0 & 0x0F, report->midiByte1, report->midiByte2);
            }
            else if ((report->midiByte0 & 0xF0) == 0x80)
            {
                offNote(report->midiByte0 & 0x0F, report->midiByte1, report->midiByte2);
            }
            break;
        }
        case LIVE_GUITAR:
        {
            PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up || report->strumBar == 0x00;
            usb_host_data->dpadDown |= down || report->strumBar == 0xFF;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            if (report->tilt != PS3_ACCEL_CENTER)
            {
                usb_host_data->tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = (report->whammy - PS3_STICK_CENTER) << 1;
            }
            break;
        }
        case DJ_HERO_TURNTABLE:
        {
            PS3Turntable_Data_t *report = (PS3Turntable_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftBlue |= report->leftBlue;
            usb_host_data->leftRed |= report->leftRed;
            usb_host_data->leftGreen |= report->leftGreen;
            usb_host_data->rightBlue |= report->rightBlue;
            usb_host_data->rightRed |= report->rightRed;
            usb_host_data->rightGreen |= report->rightGreen;
            if (report->effectsKnob != PS3_ACCEL_CENTER)
            {
                usb_host_data->effectsKnob = (report->effectsKnob - PS3_ACCEL_CENTER) << 6;
            }
            if (report->crossfader != PS3_ACCEL_CENTER)
            {
                usb_host_data->crossfader = (report->crossfader - PS3_ACCEL_CENTER) << 6;
            }
            if (report->leftTableVelocity != PS3_STICK_CENTER)
            {
                usb_host_data->leftTableVelocity = (report->leftTableVelocity - PS3_STICK_CENTER) << 8;
            }
            if (report->rightTableVelocity != PS3_STICK_CENTER)
            {
                usb_host_data->rightTableVelocity = (report->rightTableVelocity - PS3_STICK_CENTER) << 8;
            }
            break;
        }

        case ROCK_BAND_PRO_GUITAR_SQUIRE:
        case ROCK_BAND_PRO_GUITAR_MUSTANG:
        {
            PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)data;
            usb_host_data->dpadLeft |= left;
            usb_host_data->dpadRight |= right;
            usb_host_data->dpadUp |= up;
            usb_host_data->dpadDown |= down;
            TRANSLATE_PRO_GUITAR;
            break;
        }
        }
        break;
    }
    case LTEK_ID:
    {
        LTEK_Report_With_Id_Data_t *report = (LTEK_Report_With_Id_Data_t *)data;
        usb_host_data->dpadLeft |= report->dpadLeft;
        usb_host_data->dpadRight |= report->dpadRight;
        usb_host_data->dpadUp |= report->dpadUp;
        usb_host_data->dpadDown |= report->dpadDown;
        usb_host_data->start |= report->start;
        usb_host_data->back |= report->back;
        break;
    }
    case LTEK:
    {
        LTEK_Report_Data_t *report = (LTEK_Report_Data_t *)data;
        usb_host_data->dpadLeft |= report->dpadLeft;
        usb_host_data->dpadRight |= report->dpadRight;
        usb_host_data->dpadUp |= report->dpadUp;
        usb_host_data->dpadDown |= report->dpadDown;
        usb_host_data->start |= report->start;
        usb_host_data->back |= report->back;
        break;
    }
    case STEPMANIAX:
    {
        StepManiaX_Report_Data_t *report = (StepManiaX_Report_Data_t *)data;
        usb_host_data->dpadLeft |= report->dpadLeft;
        usb_host_data->dpadRight |= report->dpadRight;
        usb_host_data->dpadUp |= report->dpadUp;
        usb_host_data->dpadDown |= report->dpadDown;
        break;
    }
    case PS4:
    {
        PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)data;
        usb_host_data->dpadLeft = dpad->dpad == 6 || dpad->dpad == 5 || dpad->dpad == 7;
        usb_host_data->dpadRight = dpad->dpad == 3 || dpad->dpad == 2 || dpad->dpad == 1;
        usb_host_data->dpadUp = dpad->dpad == 0 || dpad->dpad == 1 || dpad->dpad == 7;
        usb_host_data->dpadDown = dpad->dpad == 5 || dpad->dpad == 4 || dpad->dpad == 3;
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)data;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->capture |= report->capture;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->leftStickY != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
            }
            if (report->rightStickX != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->rightStickY != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
            }
            break;
        }
        case ROCK_BAND_GUITAR:
        {
            PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            if (report->tilt)
            {
                usb_host_data->tilt = report->tilt << 7;
            }
            if (report->solo)
            {
                usb_host_data->soloGreen |= report->a;
                usb_host_data->soloRed |= report->b;
                usb_host_data->soloYellow |= report->y;
                usb_host_data->soloBlue |= report->x;
                usb_host_data->soloOrange |= report->leftShoulder;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            if (report->pickup)
            {
                usb_host_data->pickup = report->pickup;
            }
            break;
        }
        case LIVE_GUITAR:
        {
            PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadUp |= report->strumBar == 0x00;
            usb_host_data->dpadDown |= report->strumBar == 0xFF;
            if (report->tilt != PS3_ACCEL_CENTER)
            {
                usb_host_data->tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            break;
        }
        }
        break;
    }
    case PS5:
    {
        PS5Gamepad_Data_t *dpad = (PS5Gamepad_Data_t *)data;
        usb_host_data->dpadLeft = dpad->dpad == 6 || dpad->dpad == 5 || dpad->dpad == 7;
        usb_host_data->dpadRight = dpad->dpad == 3 || dpad->dpad == 2 || dpad->dpad == 1;
        usb_host_data->dpadUp = dpad->dpad == 0 || dpad->dpad == 1 || dpad->dpad == 7;
        usb_host_data->dpadDown = dpad->dpad == 5 || dpad->dpad == 4 || dpad->dpad == 3;
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)data;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->leftStickY != PS3_STICK_CENTER)
            {
                usb_host_data->leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
            }
            if (report->rightStickX != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
            }
            if (report->rightStickY != PS3_STICK_CENTER)
            {
                usb_host_data->rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
            }
            break;
        }
        case ROCK_BAND_GUITAR:
        {
            PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            if (report->tilt)
            {
                usb_host_data->tilt = report->tilt << 7;
            }
            if (report->solo)
            {
                usb_host_data->soloGreen |= report->a;
                usb_host_data->soloRed |= report->b;
                usb_host_data->soloYellow |= report->y;
                usb_host_data->soloBlue |= report->x;
                usb_host_data->soloOrange |= report->leftShoulder;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            break;
        }
        }
        break;
    }
    case SWITCH:
    {
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            SwitchProGamepad_Data_t *report = (SwitchProGamepad_Data_t *)data;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->capture |= report->capture;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != 0x800)
            {
                usb_host_data->leftStickX = (report->leftStickX - 0x800) << 4;
            }
            if (report->leftStickY != 0x800)
            {
                usb_host_data->leftStickY = (((0xFFF - report->leftStickY) - 0x800)) << 4;
            }
            if (report->rightStickX != 0x800)
            {
                usb_host_data->rightStickX = (report->rightStickX - 0x800) << 4;
            }
            if (report->rightStickY != 0x800)
            {
                usb_host_data->rightStickY = (((0xFFF - report->rightStickY) - 0x800)) << 4;
            }
            break;
        }
        case TAIKO:
        {
            SwitchTaiko_Data_t *report = (SwitchTaiko_Data_t *)data;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->dpadLeft = report->dpad == 6 || report->dpad == 5 || report->dpad == 7;
            usb_host_data->dpadRight = report->dpad == 3 || report->dpad == 2 || report->dpad == 1;
            usb_host_data->dpadUp = report->dpad == 0 || report->dpad == 1 || report->dpad == 7;
            usb_host_data->dpadDown = report->dpad == 5 || report->dpad == 4 || report->dpad == 3;
            if (report->l2)
            {
                usb_host_data->leftTrigger = UINT16_MAX;
            }
            if (report->r2)
            {
                usb_host_data->rightTrigger = UINT16_MAX;
            }
            break;
        }
        }
        break;
    }
    case SWITCH2:
    {
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            Switch2ProGamepad_Data_t *report = (Switch2ProGamepad_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->capture |= report->capture;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != SWITCH_2_STICK_CENTER)
            {
                usb_host_data->leftStickX = (report->leftStickX - SWITCH_2_STICK_CENTER) << 4;
            }
            if (report->leftStickY != SWITCH_2_STICK_CENTER)
            {
                usb_host_data->leftStickY = ((report->leftStickY - SWITCH_2_STICK_CENTER)) << 4;
            }
            if (report->rightStickX != SWITCH_2_STICK_CENTER)
            {
                usb_host_data->rightStickX = (report->rightStickX - SWITCH_2_STICK_CENTER) << 4;
            }
            if (report->rightStickY != SWITCH_2_STICK_CENTER)
            {
                usb_host_data->rightStickY = ((report->rightStickY - SWITCH_2_STICK_CENTER)) << 4;
            }
            break;
        }
        }
        break;
    }
    case OG_XBOX:
    {
        OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
        usb_host_data->a |= report->a > 0x20;
        usb_host_data->b |= report->b > 0x20;
        usb_host_data->x |= report->x > 0x20;
        usb_host_data->y |= report->y > 0x20;
        usb_host_data->leftShoulder |= report->leftShoulder > 0x20;
        usb_host_data->rightShoulder |= report->rightShoulder > 0x20;
        usb_host_data->back |= report->back;
        usb_host_data->start |= report->start;
        usb_host_data->leftThumbClick |= report->leftThumbClick;
        usb_host_data->rightThumbClick |= report->rightThumbClick;
        usb_host_data->dpadLeft = report->dpadLeft;
        usb_host_data->dpadRight = report->dpadRight;
        usb_host_data->dpadUp = report->dpadUp;
        usb_host_data->dpadDown = report->dpadDown;
        if (report->leftTrigger)
        {
            usb_host_data->leftTrigger = report->leftTrigger << 8;
        }
        if (report->rightTrigger)
        {
            usb_host_data->rightTrigger = report->rightTrigger << 8;
        }
        if (report->leftStickX)
        {
            usb_host_data->leftStickX = report->leftStickX;
        }
        if (report->leftStickY)
        {
            usb_host_data->leftStickY = report->leftStickY;
        }
        if (report->rightStickX)
        {
            usb_host_data->rightStickX = report->rightStickX;
        }
        if (report->rightStickY)
        {
            usb_host_data->rightStickY = report->rightStickY;
        }
        if (report->leftShoulder)
        {
            usb_host_data->pressureL1 = report->leftShoulder;
        }
        if (report->rightShoulder)
        {
            usb_host_data->pressureR1 = report->rightShoulder;
        }
        if (report->y)
        {
            usb_host_data->pressureTriangle = report->y;
        }
        if (report->b)
        {
            usb_host_data->pressureCircle = report->b;
        }
        if (report->a)
        {
            usb_host_data->pressureCross = report->a;
        }
        if (report->x)
        {
            usb_host_data->pressureSquare = report->x;
        }
        break;
    }
    case XBOX360_BB:
    {
        XInputBigButton_Data_t *report = (XInputBigButton_Data_t *)data;
        usb_host_data->green |= report->a;
        usb_host_data->red |= report->b;
        usb_host_data->yellow |= report->y;
        usb_host_data->blue |= report->x;
        usb_host_data->orange |= report->leftShoulder;
        usb_host_data->a |= report->a;
        usb_host_data->b |= report->b;
        usb_host_data->x |= report->x;
        usb_host_data->y |= report->y;
        usb_host_data->leftShoulder |= report->leftShoulder;
        usb_host_data->rightShoulder |= report->rightShoulder;
        usb_host_data->back |= report->back;
        usb_host_data->start |= report->start;
        usb_host_data->guide |= report->guide;
        usb_host_data->leftThumbClick |= report->leftThumbClick;
        usb_host_data->rightThumbClick |= report->rightThumbClick;
        usb_host_data->dpadLeft = report->dpadLeft;
        usb_host_data->dpadRight = report->dpadRight;
        usb_host_data->dpadUp = report->dpadUp;
        usb_host_data->dpadDown = report->dpadDown;
        break;
    }

    case XINPUTCOMPAT:
    {
        XInputCompatGamepad_Data_t *report = (XInputCompatGamepad_Data_t *)data;
        if (report->rid == 2)
        {
            usb_host_data->guide = data[1];
            break;
        }
        if (report->rid == 1)
        {
            uint8_t dpad = report->dpad - 1;
            usb_host_data->dpadLeft = dpad == 6 || dpad == 5 || dpad == 7;
            usb_host_data->dpadRight = dpad == 3 || dpad == 2 || dpad == 1;
            usb_host_data->dpadUp = dpad == 0 || dpad == 1 || dpad == 7;
            usb_host_data->dpadDown = dpad == 5 || dpad == 4 || dpad == 3;
            // Festival pro mode mappings
            usb_host_data->green |= dpad == 6 || dpad == 5 || dpad == 7;
            usb_host_data->red |= dpad == 0 || dpad == 1 || dpad == 7;
            usb_host_data->yellow |= report->x;
            usb_host_data->blue |= report->y;
            usb_host_data->orange |= report->b;
            // Festival expert mappings
            // usb_host_data->green |= dpad == 6 || dpad == 5 || dpad == 7;
            // usb_host_data->red |= dpad == 3 || dpad == 2 || dpad == 1;
            // usb_host_data->yellow |= report->x;
            // usb_host_data->blue |= report->y;
            // usb_host_data->orange |= report->b;

            // Festival easy/med/hard mappings
            // usb_host_data->green |= dpad == 6 || dpad == 5 || dpad == 7;
            // usb_host_data->red |= dpad == 3 || dpad == 2 || dpad == 1;
            // usb_host_data->yellow |= report->x;
            // usb_host_data->blue |= report->b;
            // usb_host_data->orange |= report->y;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 6;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 6;
            }
            if (report->leftStickX)
            {
                usb_host_data->leftStickX = report->leftStickX - INT16_MAX;
            }
            if (report->leftStickY)
            {
                usb_host_data->leftStickY = -(report->leftStickY - INT16_MAX);
            }
            if (report->rightStickX)
            {
                usb_host_data->rightStickX = report->rightStickX - INT16_MAX;
            }
            if (report->rightStickY)
            {
                usb_host_data->rightStickY = -(report->rightStickY - INT16_MAX);
            }
        }
        break;
    }
    case XBOX360_W:
    case XBOX360:
    {
        switch (device_type.sub_type)
        {
        case XINPUT_GUITAR_BASS:
        case XINPUT_GUITAR:
        {
            XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            if (report->tilt)
            {
                usb_host_data->tilt = INT16_MAX;
            }
            if (report->solo)
            {
                usb_host_data->soloGreen |= report->a;
                usb_host_data->soloRed |= report->b;
                usb_host_data->soloYellow |= report->y;
                usb_host_data->soloBlue |= report->x;
                usb_host_data->soloOrange |= report->leftShoulder;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = (report->whammy >> 8) - PS3_STICK_CENTER;
            }
            if (report->pickup)
            {
                usb_host_data->pickup = report->pickup;
            }
            break;
        }
        case XINPUT_GUITAR_WT:
        case XINPUT_GUITAR_ALTERNATE:
        {
            XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            if (report->tilt)
            {
                usb_host_data->tilt = report->tilt;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = (report->whammy >> 8) - PS3_STICK_CENTER;
            }

            uint8_t slider = (report->slider >> 8) ^ 0x80;

            if (device_type.sub_type == XINPUT_GUITAR_WT)
            {
                if (slider < 0x2F)
                {
                    usb_host_data->slider = 0x15;
                }
                else if (slider <= 0x3F)
                {
                    usb_host_data->slider = 0x30;
                }
                else if (slider <= 0x5F)
                {
                    usb_host_data->slider = 0x4D;
                }
                else if (slider <= 0x6F)
                {
                    usb_host_data->slider = 0x66;
                }
                else if (slider <= 0x8F)
                {
                    usb_host_data->slider = 0x80;
                }
                else if (slider <= 0x9F)
                {
                    usb_host_data->slider = 0x9A;
                }
                else if (slider <= 0xAF)
                {
                    usb_host_data->slider = 0xAF;
                }
                else if (slider <= 0xCF)
                {
                    usb_host_data->slider = 0xC9;
                }
                else if (slider <= 0xEF)
                {
                    usb_host_data->slider = 0xE6;
                }
                else
                {
                    usb_host_data->slider = 0xFF;
                }
            }
            else
            {
                usb_host_data->slider = slider;
            }
            break;
        }
        case XINPUT_DRUMS:
        {
            if (device_type.drum_type == DRUM_GH)
            {
                XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)data;
                usb_host_data->a |= report->a;
                usb_host_data->b |= report->b;
                usb_host_data->x |= report->x;
                usb_host_data->y |= report->y;
                usb_host_data->leftShoulder |= report->leftShoulder;
                usb_host_data->back |= report->back;
                usb_host_data->start |= report->start;
                usb_host_data->guide |= report->guide;
                usb_host_data->dpadLeft = report->dpadLeft;
                usb_host_data->dpadRight = report->dpadRight;
                usb_host_data->dpadUp = report->dpadUp;
                usb_host_data->dpadDown = report->dpadDown;
                SET_GH_PADS();
                // Forward any midi data we get
                if ((report->midiPacket[0] & 0xF0) == 0x90)
                {
                    onNote(report->midiPacket[0] & 0x0F, report->midiPacket[1], report->midiPacket[2]);
                }
                else if ((report->midiPacket[0] & 0xF0) == 0x80)
                {
                    offNote(report->midiPacket[0] & 0x0F, report->midiPacket[1], report->midiPacket[2]);
                }
            }
            else
            {
                XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)data;
                bool up = report->dpadUp;
                bool down = report->dpadDown;
                bool left = report->dpadLeft;
                bool right = report->dpadRight;
                bool kick1 = report->leftShoulder;
                bool kick2 = report->leftThumbClick;

                uint8_t redVelocity = xinput_rb_velocity_positive(report->redVelocity);
                uint8_t greenVelocity = xinput_rb_velocity_negative(report->greenVelocity);
                uint8_t yellowVelocity = xinput_rb_velocity_negative(report->yellowVelocity);
                uint8_t blueVelocity = xinput_rb_velocity_positive(report->blueVelocity);

                SET_RB_PADS();

                if (!report->padFlag && !report->cymbalFlag)
                {
                    usb_host_data->a |= report->a;
                    usb_host_data->b |= report->b;
                    usb_host_data->x |= report->x;
                    usb_host_data->y |= report->y;
                    usb_host_data->dpadLeft = report->dpadLeft;
                    usb_host_data->dpadRight = report->dpadRight;
                    usb_host_data->dpadUp = report->dpadUp;
                    usb_host_data->dpadDown = report->dpadDown;
                }
                usb_host_data->back |= report->back;
                usb_host_data->start |= report->start;
                usb_host_data->guide |= report->guide;
            }
            break;
        }
        case XINPUT_GUITAR_HERO_LIVE:
        {
            XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            if (report->tilt)
            {
                usb_host_data->tilt = report->tilt;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            break;
        }
        case XINPUT_TURNTABLE:
        {
            XInputTurntable_Data_t *report = (XInputTurntable_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftBlue |= report->leftBlue;
            usb_host_data->leftRed |= report->leftRed;
            usb_host_data->leftGreen |= report->leftGreen;
            usb_host_data->rightBlue |= report->rightBlue;
            usb_host_data->rightRed |= report->rightRed;
            usb_host_data->rightGreen |= report->rightGreen;
            if (report->effectsKnob)
            {
                usb_host_data->effectsKnob = report->effectsKnob;
            }
            if (report->crossfader)
            {
                usb_host_data->crossfader = report->crossfader;
            }
            if (report->leftTableVelocity)
            {
                if (report->leftTableVelocity > 127)
                {
                    report->leftTableVelocity = 127;
                }
                if (report->leftTableVelocity < -127)
                {
                    report->leftTableVelocity = -127;
                }
                usb_host_data->leftTableVelocity = report->leftTableVelocity << 8;
            }
            if (report->rightTableVelocity)
            {
                if (report->rightTableVelocity > 127)
                {
                    report->rightTableVelocity = 127;
                }
                if (report->rightTableVelocity < -127)
                {
                    report->rightTableVelocity = -127;
                }
                usb_host_data->rightTableVelocity = report->rightTableVelocity << 8;
            }
            break;
        }
        case XINPUT_PRO_KEYS:
        {
            XInputRockBandKeyboard_Data_t *report = (XInputRockBandKeyboard_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;

            TRANSLATE_PRO_KEYS;
            break;
        }
        case XINPUT_PRO_GUITAR:
        {
            XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
            TRANSLATE_PRO_GUITAR;
            break;
        }
        // Any other subtypes we dont handle can just be read like gamepads.
        default:
        {
            XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)data;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->guide |= report->guide;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX)
            {
                usb_host_data->leftStickX = report->leftStickX;
            }
            if (report->leftStickY)
            {
                usb_host_data->leftStickY = report->leftStickY;
            }
            if (report->rightStickX)
            {
                usb_host_data->rightStickX = report->rightStickX;
            }
            if (report->rightStickY)
            {
                usb_host_data->rightStickY = report->rightStickY;
            }
            break;
        }
        }
        break;
    }
    case XBOXONE:
    {
        usb_host_data->guide |= device_type.xone_guide;
        switch (device_type.sub_type)
        {
        case GAMEPAD:
        {
            XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->green |= report->a;
            usb_host_data->red |= report->b;
            usb_host_data->yellow |= report->y;
            usb_host_data->blue |= report->x;
            usb_host_data->orange |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            // XB1 reports range from 0 - 1024
            if (report->leftTrigger)
            {
                usb_host_data->leftTrigger = report->leftTrigger << 6;
            }
            if (report->rightTrigger)
            {
                usb_host_data->rightTrigger = report->rightTrigger << 6;
            }
            if (report->leftStickX)
            {
                usb_host_data->leftStickX = report->leftStickX;
            }
            if (report->leftStickY)
            {
                usb_host_data->leftStickY = report->leftStickY;
            }
            if (report->rightStickX)
            {
                usb_host_data->rightStickX = report->rightStickX;
            }
            if (report->rightStickY)
            {
                usb_host_data->rightStickY = report->rightStickY;
            }
            break;
        }
        case ROCK_BAND_GUITAR:
        {
            XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->soloGreen |= report->soloGreen;
            usb_host_data->soloRed |= report->soloRed;
            usb_host_data->soloYellow |= report->soloYellow;
            usb_host_data->soloBlue |= report->soloBlue;
            usb_host_data->soloOrange |= report->soloOrange;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            if (report->tilt)
            {
                usb_host_data->tilt = report->tilt << 7;
            }
            if (report->whammy)
            {
                usb_host_data->whammy = report->whammy;
            }
            if (report->pickup)
            {
                usb_host_data->pickup = report->pickup;
            }
            break;
        }
        case ROCK_BAND_DRUMS:
        {
            XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            if (report->greenVelocity)
            {
                onNote(10, RB_MIDI_NOTE_GREEN, report->greenVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_GREEN, 0);
            }
            if (report->redVelocity)
            {
                onNote(10, RB_MIDI_NOTE_RED, report->redVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_RED, 0);
            }
            if (report->yellowVelocity)
            {
                onNote(10, RB_MIDI_NOTE_YELLOW, report->redVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_YELLOW, 0);
            }
            if (report->blueVelocity)
            {
                onNote(10, RB_MIDI_NOTE_BLUE, report->redVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_BLUE, 0);
            }
            if (report->blueCymbalVelocity)
            {
                onNote(10, RB_MIDI_NOTE_BLUE_CYMBAL, report->redVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_BLUE_CYMBAL, 0);
            }
            if (report->yellowCymbalVelocity)
            {
                onNote(10, RB_MIDI_NOTE_YELLOW_CYMBAL, report->redVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_YELLOW_CYMBAL, 0);
            }
            if (report->greenCymbalVelocity)
            {
                onNote(10, RB_MIDI_NOTE_GREEN_CYMBAL, report->redVelocity << 3);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_GREEN_CYMBAL, 0);
            }
            if (report->leftShoulder)
            {
                onNote(10, RB_MIDI_NOTE_KICK, 0x7F);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_KICK, 0);
            }
            if (report->rightShoulder)
            {
                onNote(10, RB_MIDI_NOTE_KICK2, 0x7F);
            }
            else
            {
                offNote(10, RB_MIDI_NOTE_KICK2, 0);
            }
            break;
        }
        }
        break;
    }
    }
}

#ifdef TICK_PS2
PS2_REPORT ps2Report;
void tick_ps2output()
{
    PS2_REPORT *report = &ps2Report;
    if (!ps2_emu_tick(report))
    {
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
#include "inputs/usb_host.h"
#include "inputs/wii.h"
#include "inputs/wt_neck.h"
#include "inputs/crkd.h"

    TICK_SHARED;
    if (!startedInactivityPulse)
    {
        // check if any inputs are active, and if so set lastInputActivity to now
        for (int i = 0; i < DIGITAL_COUNT; i++)
        {
            if (debounce[i])
            {
                lastInputActivity = millis();
            }
        }
    }
    memset(report, 0, sizeof(report));
#if DEVICE_TYPE_IS_GUITAR
    report->dpadLeft = true;
    report->whammy = 0x7f;
#endif
    TICK_PS2;
    report->header = 0x5A;
}
#endif
#ifdef TICK_WII
void tick_wiioutput()
{
#include "inputs/accel.h"
#include "inputs/clone_neck.h"
#include "inputs/gh5_neck.h"
#include "inputs/mpr121.h"
#include "inputs/ps2.h"
#include "inputs/slave_tick.h"
#include "inputs/usb_host.h"
#include "inputs/wii.h"
#include "inputs/wt_neck.h"
    TICK_SHARED;
    if (!startedInactivityPulse)
    {
        // check if any inputs are active, and if so set lastInputActivity to now
        for (int i = 0; i < DIGITAL_COUNT; i++)
        {
            if (debounce[i])
            {
                lastInputActivity = millis();
            }
        }
    }
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
    if (format == 3)
    {
        memcpy(wii_data, report, sizeof(report));
    }
    else if (format == 2)
    {
        WiiClassicDataFormat2_t *real_report = (WiiClassicDataFormat2_t *)wii_data;
        real_report->buttonsLow = report->buttonsLow;
        real_report->buttonsHigh = report->buttonsHigh;
        real_report->leftStickX92 = report->leftStickX;
        real_report->leftStickY92 = report->leftStickY;
        real_report->rightStickX92 = report->rightStickX;
        real_report->rightStickY92 = report->rightStickY;
        real_report->leftTrigger = report->leftTrigger;
        real_report->rightTrigger = report->rightTrigger;
    }
    else if (format == 1)
    {
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
void bluetooth_connected()
{
    input_start = millis();
}
uint8_t rbcount = 0;
uint8_t lastrbcount = 0;
uint8_t hadCymbal = 0;
bool greenCymbal;
bool greenPad;
bool yellowCymbal;
bool blueCymbal;
long lastCymbalOff = 0;
long lastGreenOff = 0;
uint8_t tick_inputs(void *buf, USB_LastReport_Data_t *last_report, uint8_t output_console_type)
{
    uint8_t packet_size = 0;
    Buffer_Report_t current_queue_report = {val : 0};
    bool drumHit = false;
#if DIGITAL_COUNT
    bool drumSeen[DIGITAL_COUNT] = {false};
#endif
// Tick Inputs
#include "inputs/accel.h"
#include "inputs/bh_drum.h"
#include "inputs/clone_neck.h"
#include "inputs/gh5_neck.h"
#include "inputs/mpr121.h"
#include "inputs/ps2.h"
#include "inputs/slave_tick.h"
#include "inputs/usb_host.h"
#include "inputs/wii.h"
#include "inputs/wt_drum.h"
#include "inputs/wt_neck.h"
#include "inputs/crkd.h"
    // #if DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_MUSTANG || DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_SQUIRE
    //     convert_report((uint8_t *)&sysexGuitar, sizeof(sysexGuitar), {PS3, ROCK_BAND_PRO_GUITAR_SQUIRE, 0, 0, 0, false}, &usb_host_data);
    // #endif
    TICK_SHARED;
    if (!startedInactivityPulse)
    {
        // check if any inputs are active, and if so set lastInputActivity to now
        for (int i = 0; i < DIGITAL_COUNT; i++)
        {
            if (debounce[i])
            {
                lastInputActivity = millis();
            }
        }
    }

    // give the user 2 second to jump between modes (aka, hold on plug in)
    if ((millis() - input_start) < 2000 && (millis() - input_start) > 100)
    {
        TICK_DETECTION;
    }
#ifdef TICK_DETECTION_FESTIVAL
    TICK_DETECTION_FESTIVAL;
#endif
    // We tick the guitar every 5ms to handle inputs if nothing is attempting to read, but this doesn't need to output that data anywhere.
    // if input queues are enabled, then we just tick as often as possible
    if (!buf)
    {
        if (INPUT_QUEUE)
        {
            if (micros() - last_queue > 100)
            {
                last_queue = micros();
                for (int i = 0; i < DIGITAL_COUNT; i++)
                {
                    if (debounce[i])
                    {
                        debounce[i]--;
                    }
                }
#if REQUIRE_LED_DEBOUNCE
                for (int i = 0; i < LED_DEBOUNCE_COUNT; i++)
                {
                    if (ledDebounce[i])
                    {
                        ledDebounce[i]--;
                    }
                }
#endif
            }
            if (current_queue_report.val != last_queue_report.val)
            {
                queue[queue_tail] = current_queue_report;
                last_queue_report = current_queue_report;
                if (queue_size < BUFFER_SIZE_QUEUE)
                {
                    queue_size++;
                    queue_tail++;
                }
            }
        }
        return 0;
    }

    if (INPUT_QUEUE && queue_size)
    {
        current_queue_report = queue[queue_tail - queue_size];
        queue_size--;
    }
    // Tick all three reports, and then go for the first one that has changes
    // We prioritise NKRO, then Consumer, because these are both only buttons
    // Then mouse, as it is an axis so it is more likley to have changes
#if defined(TICK_NKRO) || defined(TICK_SIXKRO)
#if !DEVICE_TYPE_IS_KEYBOARD
    if (consoleType == KEYBOARD_MOUSE)
    {
#endif
        void *lastReportToCheck;
        for (int i = 1; i < REPORT_ID_END; i++)
        {
#ifdef TICK_MOUSE
            if (i == REPORT_ID_MOUSE)
            {
                packet_size = sizeof(USB_Mouse_Data_t);
                memset(buf, 0, packet_size);
                USB_Mouse_Data_t *report = (USB_Mouse_Data_t *)buf;
                report->rid = REPORT_ID_MOUSE;
                TICK_MOUSE;
                if (last_report)
                {
                    lastReportToCheck = &last_report->lastMouseReport;
                }
            }
#endif
#ifdef TICK_CONSUMER
            if (i == REPORT_ID_CONSUMER)
            {
                packet_size = sizeof(USB_ConsumerControl_Data_t);
                memset(buf, 0, packet_size);
                USB_ConsumerControl_Data_t *report = (USB_ConsumerControl_Data_t *)buf;
                report->rid = REPORT_ID_CONSUMER;
                TICK_CONSUMER;
                if (last_report)
                {
                    lastReportToCheck = &last_report->lastConsumerReport;
                }
            }
#endif
#ifdef TICK_NKRO
            if (i == REPORT_ID_NKRO)
            {
                packet_size = sizeof(USB_NKRO_Data_t);
                memset(buf, 0, packet_size);
                USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)buf;
                report->rid = REPORT_ID_NKRO;
                TICK_NKRO;
                if (last_report)
                {
                    lastReportToCheck = &last_report->lastNKROReport;
                }
            }
#endif
#ifdef TICK_SIXKRO
            if (i == REPORT_ID_NKRO)
            {
                packet_size = sizeof(USB_6KRO_Data_t);
                memset(buf, 0, packet_size);
                USB_6KRO_Data_t *report = (USB_6KRO_Data_t *)buf;
                report->rid = REPORT_ID_NKRO;
                TICK_SIXKRO;
                if (last_report)
                {
                    lastReportToCheck = &last_report->last6KROReport;
                }
            }
#endif

            // If we are directly asked for a HID report, always just reply with the NKRO one
            if (lastReportToCheck && output_console_type != BLUETOOTH_REPORT)
            {
                uint8_t cmp = memcmp(lastReportToCheck, buf, packet_size);
                if (cmp == 0)
                {
                    packet_size = 0;
                    continue;
                }
                memcpy(lastReportToCheck, buf, packet_size);
                break;
            }
            else
            {
                break;
            }
        }
        if (packet_size)
        {
            return packet_size;
        }
#endif
#if !(DEVICE_TYPE_IS_KEYBOARD)
#if defined(TICK_NKRO) || defined(TICK_SIXKRO)
    }
#endif
    USB_Report_Data_t *report_data = (USB_Report_Data_t *)buf;
    uint8_t report_size;
    bool updateSequence = false;
    bool updateHIDSequence = false;
    uint8_t proKeyVelocities[25] = {0};
    memset(proKeyVelocities, 0, sizeof(proKeyVelocities));
#if USB_HOST_STACK
    if (output_console_type == XBOXONE)
    {
        XBOX_ONE_REPORT *report = (XBOX_ONE_REPORT *)buf;
#if DEVICE_TYPE_IS_LIVE_GUITAR
        XboxOneGHLGuitarWithGamepad_Data_t *reportGHL = (XboxOneGHLGuitarWithGamepad_Data_t *)buf;
        // The GHL guitar is special. It sends both a standard gamepad report and a report with a format mirroring the ps3 ghl report
        // We append the standard gamepad after the ps3 report, so swap out buffers so that the gamepad tick writes to the correct place
        memset(buf, 0, sizeof(XboxOneGHLGuitarWithGamepad_Data_t));
        report = &reportGHL->gamepad;
#endif
        packet_size = sizeof(XBOX_ONE_REPORT);
        report_size = packet_size - sizeof(GipHeader_t);
        memset(buf, 0, packet_size);
        GIP_HEADER(report, GIP_INPUT_REPORT, false, report_sequence_number);
        TICK_XBOX_ONE;
        asm volatile("" ::
                         : "memory");

        if (report->guide != lastXboxOneGuide)
        {
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
#if DEVICE_TYPE_IS_LIVE_GUITAR
        packet_size = sizeof(XboxOneGHLGuitarWithGamepad_Data_t);
        uint8_t cmp = memcmp(last_report, report_data, report_size);
        if (cmp == 0)
        {
            // gamepad report has not changed, so only send GHL report
            packet_size = sizeof(XboxOneGHLGuitar_Data_t);
            updateSequence = false;
        }
        memcpy(last_report, report_data, report_size);
        GIP_HEADER((&reportGHL->guitar), GHL_HID_REPORT, false, hid_sequence_number);
        report_data = (USB_Report_Data_t *)&reportGHL->guitar.report;
        updateHIDSequence = true;
#endif
    }
#endif
    if (output_console_type == OG_XBOX)
    {
        OG_XBOX_REPORT *report = (OG_XBOX_REPORT *)report_data;
        memset(report_data, 0, sizeof(OG_XBOX_REPORT));
        report->rid = 0;
        report->rsize = sizeof(OG_XBOX_REPORT);
// Whammy on the xbox guitars goes from min to max, so it needs to default to min
#if DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE_IS_LIVE_GUITAR
        report->whammy = INT16_MIN;
#endif
        TICK_OG_XBOX;
        report_size = packet_size = sizeof(OG_XBOX_REPORT);
    }
    if (output_console_type == WINDOWS || output_console_type == XBOX360)
    {
        XINPUT_REPORT *report = (XINPUT_REPORT *)report_data;
        memset(report_data, 0, sizeof(XINPUT_REPORT));
        report->rid = 0;
        report->rsize = sizeof(XINPUT_REPORT);
// Whammy on the xbox guitars goes from min to max, so it needs to default to min
#if DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE_IS_LIVE_GUITAR
        report->whammy = INT16_MIN;
#endif
        TICK_XINPUT;

#if DEVICE_TYPE == GUITAR_HERO_GUITAR
        if (seen_rpcs3)
        {
            report->whammy = (INT16_MAX + (uint32_t)(report->whammy)) >> 1;
        }
#endif
#if PRO_GUITAR && USB_HOST_STACK
        TRANSLATE_TO_PRO_GUITAR(usb_host_data)
        report->autoCal_Light = report->tilt;
        report->autoCal_Microphone = report->tilt;
#endif
#if PRO_GUITAR && BLUETOOTH_RX
        TRANSLATE_TO_PRO_GUITAR(bt_data)
        report->autoCal_Light = report->tilt;
        report->autoCal_Microphone = report->tilt;
#endif
        asm volatile("" ::
                         : "memory");

#if DEVICE_TYPE == ROCK_BAND_PRO_KEYS
        bool key25 = report->key25;
        uint8_t currentVel = 0;
        for (int i = 0; i < sizeof(proKeyVelocities) && currentVel <= 4; i++)
        {
            if (proKeyVelocities[i])
            {
                report->velocities[currentVel] = proKeyVelocities[i] >> 1;
                currentVel++;
            }
        }
        report->key25 = key25;
#endif
// xb360 is stupid
#if DEVICE_TYPE == GUITAR_HERO_DRUMS
        report->leftThumbClick = true;
#endif
        report_size = packet_size = sizeof(XINPUT_REPORT);
#if DEVICE_TYPE == ROCK_BAND_DRUMS
        if (!report->cymbalFlag && ready_for_next_packet() && (greenCymbal || yellowCymbal || blueCymbal))
        {
            if (greenCymbal)
            {
                lastGreenOff = millis();
            }
            greenCymbal = false;
            yellowCymbal = false;
            blueCymbal = false;
            lastCymbalOff = millis();
        }
        if (!report->padFlag && ready_for_next_packet() && greenPad)
        {
            greenPad = false;
            lastGreenOff = millis();
        }
#endif
    }
// Guitars and Drums can fall back to their PS3 versions, so don't even include the PS4 version there.
// DJ Hero was never on ps4, so we can't really implement that either, so just fall back to PS3 there too.
#if SUPPORTS_PS4
    if (output_console_type == PS4)
    {
        if (millis() > 450000 && !auth_ps4_controller_found)
        {
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
        if (!report_requested)
        {
            report->guide = true;
        }
        TICK_PS4;

#if PRO_GUITAR && USB_HOST_STACK
        TRANSLATE_TO_PRO_GUITAR(usb_host_data)
        report->autoCal_Light = report->tilt;
        report->autoCal_Microphone = report->tilt;
#endif
#if PRO_GUITAR && BLUETOOTH_RX
        TRANSLATE_TO_PRO_GUITAR(bt_data)
        report->autoCal_Light = report->tilt;
        report->autoCal_Microphone = report->tilt;
#endif
        asm volatile("" ::
                         : "memory");
        gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
        report_size = packet_size = sizeof(PS4_REPORT);
    }
#endif
#ifdef TICK_FESTIVAL
    if (output_console_type == IOS_FESTIVAL)
    {
        if (!festival_gameplay_mode)
        {
            PS3GamepadGuitar_Data_t *report = (PS3GamepadGuitar_Data_t *)report_data;
            memset(report, 0, sizeof(PS3GamepadGuitar_Data_t));
            report->reportId = 1;
            report->accelX = PS3_ACCEL_CENTER;
            report->accelY = PS3_ACCEL_CENTER;
            report->accelZ = PS3_ACCEL_CENTER;
            report->gyro = PS3_ACCEL_CENTER;
            report->leftStickX = PS3_STICK_CENTER;
            report->leftStickY = PS3_STICK_CENTER;
            report->rightStickX = PS3_STICK_CENTER;
            report->rightStickY = PS3_STICK_CENTER;
            TICK_PS3_WITHOUT_CAPTURE;
            report_size = packet_size = sizeof(PS3GamepadGuitar_Data_t);
        }
        else
        {
            PS3FestivalProGuitarLayer_Data_t *report = (PS3FestivalProGuitarLayer_Data_t *)report_data;
            memset(report, 0, sizeof(PS3FestivalProGuitarLayer_Data_t));
            report->reportId = 0x01;
            report->leftStickX = PS3_STICK_CENTER;
            report->leftStickY = PS3_STICK_CENTER;
            report->rightStickX = PS3_STICK_CENTER;
            report->rightStickY = PS3_STICK_CENTER;
            TICK_FESTIVAL;
            report_size = packet_size = sizeof(PS3FestivalProGuitarLayer_Data_t);
        }
    }
#endif
    if (output_console_type == BLUETOOTH_REPORT || output_console_type == UNIVERSAL)
    {
#ifdef TICK_FESTIVAL
        if (!festival_gameplay_mode)
        {
#endif
            PC_REPORT *report = (PC_REPORT *)report_data;
            report_size = packet_size = sizeof(PC_REPORT);
            memset(report, 0, sizeof(PC_REPORT));
#if DEVICE_TYPE == GAMEPAD
            report->leftStickX = PS3_STICK_CENTER;
            report->leftStickY = PS3_STICK_CENTER;
            report->rightStickX = PS3_STICK_CENTER;
            report->rightStickY = PS3_STICK_CENTER;
#endif
#if DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE_IS_LIVE_GUITAR
            report->tilt = PS3_STICK_CENTER;
            report->whammy = 0;
#endif
#if DEVICE_TYPE == GUITAR_HERO_GUITAR
            report->slider = PS3_STICK_CENTER;
#endif
#if DEVICE_TYPE == ROCK_BAND_GUITAR
            report->pickup = PS3_STICK_CENTER;
#endif
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
            report->leftTableVelocity = PS3_STICK_CENTER;
            report->rightTableVelocity = PS3_STICK_CENTER;
            report->crossfader = PS3_STICK_CENTER;
            report->effectsKnob = PS3_STICK_CENTER;
#endif
            report->reportId = 1;
            TICK_PC;
#if DEVICE_TYPE == GUITAR_HERO_GUITAR
            if (seen_rpcs3)
            {
                report->whammy = PS3_STICK_CENTER + (report->whammy >> 1);
            }
#endif

#if PRO_GUITAR && USB_HOST_STACK
            TRANSLATE_TO_PRO_GUITAR(usb_host_data)
#endif
#if PRO_GUITAR && BLUETOOTH_RX
            TRANSLATE_TO_PRO_GUITAR(bt_data)
#endif
            asm volatile("" ::
                             : "memory");
            report->dpad = (report->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[report->dpad];
#ifdef TICK_FESTIVAL
        }
        else
        {
            PC_FESTIVAL_REPORT *report = (PC_FESTIVAL_REPORT *)report_data;
            report_size = packet_size = sizeof(PC_FESTIVAL_REPORT);
            memset(report, 0, sizeof(PC_FESTIVAL_REPORT));
            report->unused[0] = PS3_STICK_CENTER;
            report->unused[1] = PS3_STICK_CENTER;
            report->slider = PS3_STICK_CENTER;
            report->reportId = 1;
            TICK_FESTIVAL;

            asm volatile("" ::
                             : "memory");
            report->dpad = (report->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[report->dpad];
        }
#endif
    }

#if DEVICE_TYPE_IS_GUITAR
    if (output_console_type == FNF)
    {
        report_size = packet_size = sizeof(PCFortniteRockBandGuitar_Data_t);
        PCFortniteRockBandGuitar_Data_t *report = (PCFortniteRockBandGuitar_Data_t *)report_data;
        memset(report, 0, sizeof(PCFortniteRockBandGuitar_Data_t));

#if DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE_IS_LIVE_GUITAR
        report->tilt = PS3_STICK_CENTER;
#endif
        report->reportId = GIP_INPUT_REPORT;
        TICK_XBOX_ONE;
        asm volatile("" ::
                         : "memory");
        //  alias tilt to dpad left so that tilt works
        if (report->tilt > 200 || report->back)
        {
            report->dpadLeft = true;
        }
        report->back = false;
    }
    if (output_console_type == ARCADE)
    {
        report_size = packet_size = sizeof(ArcadeGuitarHeroGuitar_Data_t);
        ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)report_data;
        memset(report, 0, sizeof(ArcadeGuitarHeroGuitar_Data_t));
        report->always_1d = 0x1d;
        report->always_ff = 0xff;
        report->side = arcadeSide;
        TICK_ARCADE;
        asm volatile("" ::
                         : "memory");
        if (report->dpadUp)
        {
            report->dpad = 0;
        }
        else if (report->dpadDown)
        {
            report->dpad = 2;
        }
        else if (report->dpadLeft)
        {
            report->dpad = 3;
        }
        else if (report->dpadRight)
        {
            report->dpad = 1;
        }
        else
        {
            report->dpad = 0xf;
        }
    }
#endif
// For gamepads, use the PS3 report format on PS3
#if DEVICE_TYPE == GAMEPAD
    if (output_console_type == PS3)
    {
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)report_data;
        memset(report, 0, sizeof(PS3Gamepad_Data_t));
        report->reportId = 1;
        report->accelX = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->accelY = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->accelZ = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->gyro = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->leftStickX = PS3_STICK_CENTER;
        report->leftStickY = PS3_STICK_CENTER;
        report->rightStickX = PS3_STICK_CENTER;
        report->rightStickY = PS3_STICK_CENTER;
        TICK_PS3_WITHOUT_CAPTURE;
        report_size = packet_size = sizeof(PS3Gamepad_Data_t);
    }
    if (output_console_type != OG_XBOX && output_console_type != WINDOWS && output_console_type != XBOX360 && output_console_type != PS3 && output_console_type != BLUETOOTH_REPORT && output_console_type != UNIVERSAL && output_console_type != XBOXONE && output_console_type != PS4)
    {
#else
    // For instruments, we instead use the below block, as all other console types use the below format
    if ((output_console_type != OG_XBOX && output_console_type != ARCADE && output_console_type != WINDOWS && output_console_type != KEYBOARD_MOUSE && output_console_type != IOS_FESTIVAL && output_console_type != FNF && output_console_type != XBOX360 && output_console_type != PS4 && output_console_type != BLUETOOTH_REPORT && output_console_type != UNIVERSAL && output_console_type != XBOXONE) || updateHIDSequence)
    {
#endif
        report_size = sizeof(PS3_REPORT);
        // Do NOT update the size for XBONE, since the XBONE packets have a totally different size!
        if (!updateHIDSequence)
        {
            packet_size = report_size;
        }

        PS3Dpad_Data_t *gamepad = (PS3Dpad_Data_t *)report_data;
        memset(gamepad, 0, sizeof(PS3Dpad_Data_t));

        asm volatile("" ::
                         : "memory");
        gamepad->accelX = PS3_ACCEL_CENTER;
        gamepad->accelY = PS3_ACCEL_CENTER;
        gamepad->accelZ = PS3_ACCEL_CENTER;
        gamepad->gyro = PS3_ACCEL_CENTER;
        gamepad->leftStickX = PS3_STICK_CENTER;
        gamepad->leftStickY = PS3_STICK_CENTER;
#if DEVICE_TYPE != ROCK_BAND_PRO_KEYS
        // For Pro Keys, these bytes should be 0 by default
        gamepad->rightStickX = PS3_STICK_CENTER;
        gamepad->rightStickY = PS3_STICK_CENTER;
#endif

#if DEVICE_TYPE == GUITAR_HERO_GUITAR
        if (output_console_type == PS2_ON_PS3)
        {
            PS2GuitarOnPS3_Data_t *report = (PS2GuitarOnPS3_Data_t *)report_data;

            TICK_PS3;
            report->whammy = 0xFF - report->whammy;
            report->dpadLeft = true;

            asm volatile("" ::
                             : "memory");

            gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
        }
        else
        {
#endif
#ifdef TICK_FESTIVAL
            if (!festival_gameplay_mode)
            {
#endif
                PS3_REPORT *report = (PS3_REPORT *)report_data;
#if DEVICE_TYPE == ROCK_BAND_GUITAR
                report->whammy = 0;
#endif
                TICK_PS3;

#if PRO_GUITAR && USB_HOST_STACK
                TRANSLATE_TO_PRO_GUITAR(usb_host_data)
                report->autoCal_Light = report->tilt;
                report->autoCal_Microphone = report->tilt;
#endif
#if PRO_GUITAR && BLUETOOTH_RX
                TRANSLATE_TO_PRO_GUITAR(bt_data)
                report->autoCal_Light = report->tilt;
                report->autoCal_Microphone = report->tilt;
#endif
                asm volatile("" ::
                                 : "memory");
#ifdef TICK_FESTIVAL
                if (output_console_type == SWITCH)
                {
                    gamepad->accelX = PS3_ACCEL_CENTER;
                    gamepad->accelY = PS3_ACCEL_CENTER;
                    gamepad->accelZ = PS3_ACCEL_CENTER;
                    gamepad->gyro = PS3_ACCEL_CENTER;
                    gamepad->leftStickX = PS3_STICK_CENTER;
                    gamepad->leftStickY = PS3_STICK_CENTER;
                    gamepad->rightStickX = PS3_STICK_CENTER;
                    gamepad->rightStickY = PS3_STICK_CENTER;
                }
#endif

#if DEVICE_TYPE == ROCK_BAND_DRUMS
                if (!report->cymbalFlag && ready_for_next_packet() && (greenCymbal || yellowCymbal || blueCymbal))
                {
                    if (greenCymbal)
                    {
                        lastGreenOff = millis();
                    }
                    greenCymbal = false;
                    yellowCymbal = false;
                    blueCymbal = false;
                    lastCymbalOff = millis();
                }
                if (!report->padFlag && ready_for_next_packet() && greenPad)
                {
                    greenPad = false;
                    lastGreenOff = millis();
                }
#endif

#if DEVICE_TYPE == ROCK_BAND_PRO_KEYS
                bool key25 = report->key25;
                uint8_t currentVel = 0;
                for (int i = 0; i < sizeof(proKeyVelocities) && currentVel <= 4; i++)
                {
                    if (proKeyVelocities[i])
                    {
                        report->velocities[currentVel] = proKeyVelocities[i] >> 1;
                        currentVel++;
                    }
                }
                report->key25 = key25;
#endif
                gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
                if (SWAP_SWITCH_FACE_BUTTONS && output_console_type == SWITCH)
                {
                    bool a = gamepad->a;
                    bool b = gamepad->b;
                    bool x = gamepad->x;
                    bool y = gamepad->y;
                    gamepad->b = a;
                    gamepad->a = b;
                    gamepad->x = y;
                    gamepad->y = x;
                }
#ifdef TICK_FESTIVAL
            }
            else
            {
                SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)report_data;
                TICK_FESTIVAL;

                asm volatile("" ::
                                 : "memory");
                gamepad->dpad = (gamepad->dpad & 0xf) > 0x0a ? 0x08 : dpad_bindings[gamepad->dpad];
            }
#endif

#if DEVICE_TYPE == GUITAR_HERO_GUITAR
        }
#endif
    }
    TICK_RESET
    // Some hosts want packets sent every frame
    if (last_report && output_console_type != ARCADE && output_console_type != OG_XBOX && output_console_type != PS4 && output_console_type != IOS_FESTIVAL && output_console_type != PS3 && output_console_type != BLUETOOTH_REPORT && output_console_type != XBOX360 && !updateHIDSequence)
    {
        uint8_t cmp = memcmp(last_report, report_data, report_size);
        if (cmp == 0)
        {
            return 0;
        }
        memcpy(last_report, report_data, report_size);
    }
// Standard PS4 controllers need a report counter, but we don't want to include that when comparing so we add it here
#if SUPPORTS_PS4
    if (output_console_type == PS4)
    {
        PS4Gamepad_Data_t *gamepad = (PS4Gamepad_Data_t *)report_data;
        gamepad->reportCounter = ps4_sequence_number++;
    }
#endif

#if USB_HOST_STACK
#if DEVICE_TYPE_IS_GAMEPAD
    if (updateSequence)
    {
        report_sequence_number++;
        if (report_sequence_number == 0)
        {
            report_sequence_number = 1;
        }
    }
#if DEVICE_TYPE_IS_LIVE_GUITAR
    if (updateHIDSequence)
    {
        hid_sequence_number++;
        if (hid_sequence_number == 0)
        {
            hid_sequence_number = 1;
        }
    }
#endif
#endif
#endif
#endif
    return packet_size;
}

#ifdef BLUETOOTH_TX
bool tick_bluetooth(void)
{
    uint8_t size = tick_inputs(&bt_report, &last_report_bt, BLUETOOTH_REPORT);
    send_report(size, (uint8_t *)&bt_report);
    return size;
}
#endif
bool windows_in_hid = false;
unsigned long millis_at_boot = 0;
uint8_t init_packet_skylanders[32] = {0x0b, 0x14, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
uint8_t version_response_skylanders[32] = {0x0b, 0x14, 0x52, 0x02, 0x1B};
bool tick_usb(void)
{
    uint8_t size = 0;
    bool ready = ready_for_next_packet();
#ifdef BLUETOOTH_TX
    if (!ready)
    {
        return false;
    }
#endif
    if (millis_at_boot == 0 && read_device_desc)
    {
        millis_at_boot = millis();
    }

#if !DEVICE_TYPE_IS_KEYBOARD
    if (!WINDOWS_USES_XINPUT)
    {
        // If we ended up here, then someone probably changed back to hid mode so we should jump back
        if (consoleType == WINDOWS)
        {
            consoleType = UNIVERSAL;
            reset_usb();
        }
    }
#endif
#if DEVICE_TYPE_IS_GAMEPAD
    // PS2 / Wii / WiiU / Switch 2 won't do WCID and wont try to read any actual strings
    if (millis_at_boot && (millis() - millis_at_boot) > 2000 && consoleType == UNIVERSAL && !seen_windows_xb1 && !seen_os_descriptor_read && !read_any_device_string)
    {
        // og xbox does its own thing
        if (seen_og_xbox)
        {
            consoleType = OG_XBOX;
            reset_usb();
        }
        // Switch 2 does read the hid descriptor
        if (seen_hid_descriptor_read)
        {
            consoleType = SWITCH;
            reset_usb();
        }
        // PS2 / Wii / WiiU does not read hid descriptor
        // The wii however will configure the usb device before it stops communicating
#if DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == ROCK_BAND_DRUMS
        if (usb_configured())
        {
            set_console_type(WII_RB);
        }
#endif
        // But the PS2 does not. We also end up here on the wii/wiiu if a device does not have an explicit wii mode.
        set_console_type(PS3);
    }
    // Due to some quirks with how the PS3 detects controllers, we can also end up here for PS3, but in that case, we won't see any requests for controller data
    if ((millis() - millis_at_boot) > 2000 && consoleType == PS4 && !seen_ps4)
    {
        set_console_type(PS3);
    }
#endif
    if (!ready)
        return 0;
#if USB_HOST_STACK
    if (data_from_console_size)
    {
        USB_Device_Type_t type = get_device_address_for(XBOXONE);
        send_report_to_controller(type.dev_addr, type.instance, data_from_console, data_from_console_size);
        data_from_console_size = 0;
    }
    // If we have something pending to send to the xbox one controller, send it
    if (consoleType == XBOXONE && xbox_one_state != Ready)
    {
        size = tick_xbox_one();
        if (!size)
        {
            // We don't want the controller ticked due to usb being stated as "not ready"
            return true;
        }
    }
// #if DEVICE_TYPE == SKYLANDERS
//     if (consoleType == XBOX360) {
//         size = 32;
//         switch (portal_state) {
//             case 0:
//                 memcpy(&combined_report, init_packet_skylanders, sizeof(init_packet_skylanders));
//                 break;
//             case 1:
//                 memcpy(&combined_report, version_response_skylanders, sizeof(version_response_skylanders));
//                 break;
//         }
//     }
// if (consoleType == XBOXONE) {
//     size = 32;
//     switch (portal_state) {
//         case 0:
//             memcpy(&combined_report, init_packet_skylanders, sizeof(init_packet_skylanders));
//             break;
//         case 1:
//             memcpy(&combined_report, version_response_skylanders, sizeof(version_response_skylanders));
//             break;
//     }
// }
// #endif
#endif
    if (!size)
    {
        size = tick_inputs(&combined_report, &last_report_usb, consoleType);
    }
    if (size)
    {
        lastSentPacket = millis(); // make 5ms idle ticking wait until 5ms after last report
        send_report_to_pc(&combined_report, size);
    }
    report_requested = true;
    return size;
}
#if BLUETOOTH_RX
int tick_bluetooth_inputs(const void *data, uint8_t len, USB_Device_Type_t device_type)
{
    memset(&bt_data, 0, sizeof(bt_data));
    bt_data.slider = 0x80;
    convert_report((const uint8_t *)data, len, device_type, &bt_data);
    return 0;
}
#endif
void tick(void)
{
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

#include "inputs/turntable.h"
    if (reset_after_360 && millis() > reset_after_360)
    {
        reset_usb();
    }
    // Only sleep if the timeout has passed and we aren't connected over USB
    if (SLEEP_PIN != -1 && SLEEP_INACTIVITY_TIMEOUT_SEC && millis() - lastInputActivity >= ((uint32_t)SLEEP_INACTIVITY_TIMEOUT_SEC * 1000) && !usb_configured())
    {
        go_to_sleep();
    }

    // Tick inputs constantly for detection for 2 seconds
    // Tick inputs constantly for xbox 360 wakeup if usb isnt configured
    if ((millis() - input_start) < 2000 || !usb_configured())
    {
        tick_inputs(NULL, NULL, consoleType);
    }
#if DEVICE_TYPE != ROCK_BAND_PRO_KEYS
    if ((millis() - lastMidi) > 1000)
    {
        memset(midiData.midiVelocities, 0, sizeof(midiData.midiVelocities));
        lastMidi = millis();
    }
#endif

#ifdef TICK_LED_PERIPHERAL
    // If we are controlling peripheral leds, then we need to send the latest state when
    // the device is plugged in again
    if (slave_initted)
    {
        if (RGB_INACTIVITY_TIMEOUT_SEC && millis() - lastInputActivity >= (((uint32_t)RGB_INACTIVITY_TIMEOUT_SEC) * 1000))
        {
            memset(ledStatePeripheral, 0, sizeof(ledStatePeripheral));
        }
        if (memcmp(lastLedStatePeripheral, ledStatePeripheral, sizeof(ledStatePeripheral)) != 0)
        {
            memcpy(lastLedStatePeripheral, ledStatePeripheral, sizeof(ledStatePeripheral));
            TICK_LED_PERIPHERAL;
        }
    }
    else
    {
        memset(lastLedStatePeripheral, 0, sizeof(lastLedStatePeripheral));
    }
#endif
#ifdef TICK_LED
    if (RGB_INACTIVITY_TIMEOUT_SEC && millis() - lastInputActivity >= (((uint32_t)RGB_INACTIVITY_TIMEOUT_SEC) * 1000))
    {
        memset(ledState, 0, sizeof(ledState));
    }
    if (memcmp(lastLedState, ledState, sizeof(ledState)) != 0)
    {
        memcpy(lastLedState, ledState, sizeof(ledState));
        TICK_LED;
    }
#endif

#if LED_COUNT_MPR121
    if (RGB_INACTIVITY_TIMEOUT_SEC && millis() - lastInputActivity >= (((uint32_t)RGB_INACTIVITY_TIMEOUT_SEC) * 1000))
    {
        ledStateMpr121 = 0;
    }
    if (lastLedStateMpr121 != ledStateMpr121)
    {
        lastLedStateMpr121 = ledStateMpr121;
        twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_GPIODATA, ledStateMpr121);
    }
#endif
#ifdef TICK_PS2
    tick_ps2output();
#endif
    if (!INPUT_QUEUE && micros() - lastDebounce > 1000)
    {
        // No benefit to ticking bluetooth faster than this!
#ifdef BLUETOOTH_TX
        tick_bluetooth();
#endif
        lastDebounce = micros();
        for (int i = 0; i < DIGITAL_COUNT; i++)
        {
            if (debounce[i])
            {
                debounce[i]--;
            }
        }
#if REQUIRE_LED_DEBOUNCE
        for (int i = 0; i < LED_DEBOUNCE_COUNT; i++)
        {
            if (ledDebounce[i])
            {
                ledDebounce[i]--;
            }
        }
#endif
    }
#if DEVICE_TYPE_IS_GUITAR
    if (consoleType == KEYBOARD_MOUSE || consoleType == FNF)
    {
        if (!INPUT_QUEUE && (micros() - last_poll) < (4000))
        {
            return;
        }
    }
#endif
    if (!INPUT_QUEUE && POLL_RATE && (micros() - last_poll) < (POLL_RATE * 1000))
    {
        return;
    }
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
    if (consoleType == PS3)
    {
        if (!INPUT_QUEUE && (micros() - last_poll_dj_ps3) < (10000))
        {
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
    if (INPUT_QUEUE && !ready)
    {
        tick_inputs(NULL, NULL, consoleType);
    }

    last_poll = micros();
    // Tick the controller every 1ms if this device is connected to a usb port and usb is not ready
    if (!INPUT_QUEUE && !ready && usb_configured() && millis() - lastSentPacket >= 1)
    {
        lastSentPacket = millis();
        tick_inputs(NULL, NULL, consoleType);
    }

#if HEARTBEAT_PERIOD_MS
    if ((lastHeartbeat == 0) || (millis() - lastHeartbeat >= HEARTBEAT_PERIOD_MS))
    {
        // heartbeat resets regularly regardless of activity
        lastHeartbeat = millis();
        // trigger output only if inputs have been active though
        if (millis() - lastInputActivity < HEARTBEAT_INACTIVITY_TIMEOUT_MS)
        {
            uint8_t activeMask = HEARTBEAT_INVERT ? 0 : HEARTBEAT_GPIO_MASK;
            digital_write(HEARTBEAT_GPIO_PORT, HEARTBEAT_GPIO_MASK, activeMask);
        }
    }
    // unset heartbeat output if pulse is over
    if (millis() - lastHeartbeat >= HEARTBEAT_PULSE_DURATION_MS)
    {
        uint8_t activeMask = HEARTBEAT_INVERT ? HEARTBEAT_GPIO_MASK : 0;
        digital_write(HEARTBEAT_GPIO_PORT, HEARTBEAT_GPIO_MASK, activeMask);
    }
#endif // HEARTBEAT_PERIOD_MS

#if INACTIVITY_OUTPUT_TIMEOUT_MS
#if INACTIVITY_OUTPUT_NON_USB_ONLY
    if (!usb_configured())
    {
#endif // INACTIVITY_OUTPUT_NON_USB_ONLY
       // unlike heartbeat, no regular resetting cycle - just a timeout for the
       // inactivity output
        if (millis() - lastInputActivity >= INACTIVITY_OUTPUT_TIMEOUT_MS)
        {
#if INACTIVITY_OUTPUT_PULSE_COUNT
            // gate to check if already done outputting pulses
            // (technically not super ideal because lastInactivityPulse may slightly desynchronise,
            // but should be fine for intended purpose of a few fairly long pulses)
            if (millis() - lastInputActivity - INACTIVITY_OUTPUT_TIMEOUT_MS < INACTIVITY_OUTPUT_PULSE_TOTAL_MS)
            {
                startedInactivityPulse = true;
                if (millis() - lastInactivityPulse >= INACTIVITY_OUTPUT_PULSE_PERIOD_MS)
                {
                    lastInactivityPulse = millis();
                    // input activity has already been checked, so no need to recheck
                    // - just set the output
                    uint8_t activeMask = INACTIVITY_OUTPUT_INVERT ? 0 : INACTIVITY_OUTPUT_GPIO_MASK;
                    digital_write(INACTIVITY_OUTPUT_GPIO_PORT, INACTIVITY_OUTPUT_GPIO_MASK, activeMask);
                }
                // unset output once current pulse's duration is over
                if (millis() - lastInactivityPulse >= INACTIVITY_OUTPUT_PULSE_DURATION_MS)
                {
                    uint8_t activeMask = INACTIVITY_OUTPUT_INVERT ? INACTIVITY_OUTPUT_GPIO_MASK : 0;
                    digital_write(INACTIVITY_OUTPUT_GPIO_PORT, INACTIVITY_OUTPUT_GPIO_MASK, activeMask);
                }
            }
            else
            {
                startedInactivityPulse = false;
                // unset output once all pulses are done
                uint8_t activeMask = INACTIVITY_OUTPUT_INVERT ? INACTIVITY_OUTPUT_GPIO_MASK : 0;
                digital_write(INACTIVITY_OUTPUT_GPIO_PORT, INACTIVITY_OUTPUT_GPIO_MASK, activeMask);
            }
#else  // INACTIVITY_OUTPUT_PULSE_COUNT
       // set output continuously on if no pulses
        uint8_t activeMask = INACTIVITY_OUTPUT_INVERT ? 0 : INACTIVITY_OUTPUT_GPIO_MASK;
        digital_write(INACTIVITY_OUTPUT_GPIO_PORT, INACTIVITY_OUTPUT_GPIO_MASK, activeMask);
#endif // INACTIVITY_OUTPUT_PULSE_COUNT
        }
        else
        {
            startedInactivityPulse = false;
            // unset output when inputs have been active
            uint8_t activeMask = INACTIVITY_OUTPUT_INVERT ? INACTIVITY_OUTPUT_GPIO_MASK : 0;
            digital_write(INACTIVITY_OUTPUT_GPIO_PORT, INACTIVITY_OUTPUT_GPIO_MASK, activeMask);
        }
#if INACTIVITY_OUTPUT_NON_USB_ONLY
    }
#endif // INACTIVITY_OUTPUT_NON_USB_ONLY
#endif // INACTIVITY_OUTPUT_TIMEOUT_MS
}

void device_reset(void)
{
#if USB_HOST_STACK
    if (consoleType == XBOXONE)
    {
        if (xbox_one_state != Announce && xbox_one_state != WaitingDesc1)
        {
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
void receive_report_from_controller(uint8_t const *report, uint16_t len)
{
    if (report[0] == GIP_INPUT_REPORT)
    {
        report_sequence_number = report[2] + 1;
    }
    const GipHeader_t *header = (const GipHeader_t *)report;
    if (header->command == GIP_ARRIVAL)
    {
        return;
    }
    if (xbox_one_state != Auth)
    {
        return;
    }
    data_from_controller_size = len;
    memcpy(data_from_controller, report, len);
}

void xinput_controller_connected(uint16_t vid, uint16_t pid)
{
    handle_player_leds(0);
    if (xbox_360_state == Authenticated)
        return;
}

void xinput_w_controller_connected()
{
    handle_player_leds(0);
}

void xone_controller_connected(uint8_t dev_addr, uint8_t instance)
{
    printf("Sending to controller %d\r\n", dev_addr);
    send_report_to_controller(dev_addr, instance, (uint8_t *)&powerMode, sizeof(GipPowerMode_t));
}
bool xone_controller_send_init_packet(uint8_t dev_addr, uint8_t instance, uint8_t id)
{
    if (id == 0)
    {
        send_report_to_controller(dev_addr, instance, (uint8_t *)&ledOn, sizeof(ledOn));
        return true;
    }
    else if (id == 1 && consoleType != XBOXONE)
    {
        // When used for auth, the console will send this
        send_report_to_controller(dev_addr, instance, (uint8_t *)&authDonePacket, sizeof(authDonePacket));
        return true;
    }
    return false;
}

void host_controller_connected()
{
    // With input_usb_host, we need to run detection for a little bit after the input is detected
#ifdef INPUT_USB_HOST
    input_start = millis();
#endif
}

void ps4_controller_connected(uint8_t dev_addr, uint16_t vid, uint16_t pid)
{
    if (vid == SONY_VID && (pid == PS4_DS_PID_1 || pid == PS4_DS_PID_2 || pid == PS4_DS_PID_3))
    {
        handle_player_leds(0);
    }
    auth_ps4_controller_found = true;
}

void ps3_controller_connected(uint8_t dev_addr, uint16_t vid, uint16_t pid)
{
    if (vid == SONY_VID && pid == SONY_DS3_PID)
    {
        // Enable PS3 reports
        uint8_t hid_command_enable[] = {0x42, 0x0c, 0x00, 0x00};
        transfer_with_usb_controller(dev_addr, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x03F4, 0x00, sizeof(hid_command_enable), hid_command_enable, NULL);
        handle_player_leds(0);
    }
}

void ps4_controller_disconnected(void)
{
    auth_ps4_controller_found = false;
}

void set_console_type(uint8_t new_console_type)
{
    if (consoleType == new_console_type && new_console_type != UNIVERSAL)
        return;
    consoleType = new_console_type;
    reset_usb();
}
#if USB_HOST_STACK || BLUETOOTH_RX
void get_usb_device_type_for(uint16_t vid, uint16_t pid, uint16_t version, USB_Device_Type_t *type)
{
    switch (vid)
    {
    case STREAM_DECK_VID:
    {
        type->console_type = STREAM_DECK;
        type->sub_type = UNKNOWN;
        switch (pid)
        {
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
    case NINTENDO_VID:
    {
        if (pid == SWITCH_PRO_PID)
        {
            type->console_type = SWITCH;
        }
        if (pid == SWITCH_2_PRO_PID)
        {
            type->console_type = SWITCH2;
        }
        if (pid == SWITCH_2_GC_PID)
        {
            type->console_type = SWITCH2;
        }
        if (pid == SWITCH_2_JOY_R_PID)
        {
            type->console_type = SWITCH2;
        }
        if (pid == SWITCH_2_JOY_L_PID)
        {
            type->console_type = SWITCH2;
        }
        break;
    }
    case ARDWIINO_VID:
    {
        if (pid == ARDWIINO_PID)
        {
            type->console_type = SANTROLLER;
            type->sub_type = (version >> 8) & 0xFF;
        }
        break;
    }
    case RAPHNET_VID:
    {
        type->console_type = RAPHNET;
        type->sub_type = GAMEPAD;
        break;
    }
    case MAGICBOOTS_PS4_VID:
    {
        if (pid == MAGICBOOTS_PS4_PID)
        {
            type->console_type = PS4;
            type->sub_type = GAMEPAD;
        }
        break;
    }
    case ARDUINO_VID:
    {
        if (pid == STEPMANIA_X_PID)
        {
            type->console_type = STEPMANIAX;
            type->sub_type = DANCE_PAD;
        }
        break;
    }
    case LTEK_LUFA_VID:
    {
        if (pid == LTEK_LUFA_PID)
        {
            type->console_type = LTEK_ID;
            type->sub_type = DANCE_PAD;
        }
        break;
    }
    case LTEK_VID:
    {
        if (pid == LTEK_PID)
        {
            type->console_type = LTEK;
            type->sub_type = DANCE_PAD;
        }
        break;
    }
    case SONY_VID:
        switch (pid)
        {
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
    case STADIA_VID:
        if (pid == STADIA_PID)
        {
            type->console_type = STADIA;
            type->sub_type = GAMEPAD;
        }
        break;
    case REDOCTANE_VID:
        switch (pid)
        {
        case PS3_GH_GUITAR_PID:
            type->console_type = PS3;
            type->sub_type = GUITAR_HERO_GUITAR;
            break;
        case PS3_GH_DRUM_PID:
            type->console_type = PS3;
            type->sub_type = GUITAR_HERO_DRUMS;
            type->drum_type = DRUM_GH;
            break;
        case PS3_RB_GUITAR_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_GUITAR;
            break;
        case PS3_RB_DRUM_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_DRUMS;
            type->drum_type = version == 0x1000 ? DRUM_RB1 : DRUM_RB2;
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
        case PS3_MUSTANG_MPA_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_PRO_GUITAR_MUSTANG;
            break;
        case PS3_SQUIRE_PID:
        case PS3_SQUIRE_MPA_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_PRO_GUITAR_SQUIRE;
            break;
        }
        break;
    case HORI_VID:
        switch (pid)
        {
        case HORI_SWITCH_TAIKO_PID:
            type->console_type = SWITCH;
            type->sub_type = TAIKO;
            break;
        }

    case HARMONIX_VID:
        // Polled the same as PS3, so treat them as PS3 instruments
        switch (pid)
        {
        case WII_RB_GUITAR_PID:
        case WII_RB_GUITAR_2_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_GUITAR;
            break;

        case WII_RB_DRUM_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_DRUMS;
            type->drum_type = DRUM_RB1;
            break;
        case WII_RB_DRUM_2_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_DRUMS;
            type->drum_type = DRUM_RB2;
            break;
        case WII_KEYBOARD_PID:
        case WII_MPA_KEYBOARD_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_PRO_KEYS;
            break;
        case WII_MUSTANG_PID:
        case WII_MUSTANG_MPA_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_PRO_GUITAR_MUSTANG;
            break;
        case WII_SQUIRE_PID:
        case WII_SQUIRE_MPA_PID:
            type->console_type = PS3;
            type->sub_type = ROCK_BAND_PRO_GUITAR_SQUIRE;
            break;
        case XBOX_360_ION_ROCKER_VID:
            type->console_type = XBOX360;
            type->sub_type = ROCK_BAND_DRUMS;
            type->drum_type = DRUM_RB2;
            break;
        }

        break;
    case PDP_VID:
        switch (pid)
        {
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
        switch (pid)
        {
        case XBOX_ONE_RB_GUITAR_PID:
            type->console_type = XBOXONE;
            type->sub_type = ROCK_BAND_GUITAR;
            break;
        case XBOX_ONE_RB_DRUM_PID:
            type->console_type = XBOXONE;
            type->sub_type = ROCK_BAND_DRUMS;
            type->drum_type = DRUM_RB2;
            break;
        case PS4_STRAT_PID:
            type->console_type = PS4;
            type->sub_type = ROCK_BAND_GUITAR;
            break;
        }

        break;

    case XBOX_REDOCTANE_VID:
        switch (pid)
        {
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
    case XBOX_ONE_CONTROLLER_VID:
        if (pid >= 0x02e0 && pid <= 0x02eF && type->console_type != XBOXONE)
        {
            type->console_type = XINPUTCOMPAT;
            type->sub_type = GAMEPAD;
            break;
        }
        break;
    }
}
#endif
