#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"


#define CONSOLE_TYPE 0
#define DEVICE_TYPE 7
#define POLL_RATE 1

#define LED_TYPE 1

#define TILT_ENABLED true

#define RF_ENABLED false
#define RF_ID 0

#define DIGITAL_COUNT 13
#define ADC_COUNT 3
#define ADC_PINS {7,6,5}

#define SKIP_MASK_PICO 0

#define SKIP_MASK_AVR {15, 0, 3, 0, 0}

#define PIN_INIT uint8_t oldSREG = SREG;cli();PORTD = 147;PORTF = 16;PORTB = 48;PORTE = 64;PORTC = 64;DDRD = 0;DDRF = 0;DDRB = 0;DDRE = 0;DDRC = 0;SREG = oldSREG;

#define TICK report->l_x = adc(1, 15, 70, 19); report->l_y = adc(2, 15, 69, 24); report->r_x = adc_trigger(0, 3, -203, 11); report->r_y = ((PINB & (1 << 6)) == 0) * 32767; if (((PIND & (1 << 0)) == 0)) {debounce[0] = 5;report->buttons |= (1 << 9);} else if (debounce[0]) { debounce[0]--; report->buttons |= (1 << 9);}if (((PIND & (1 << 1)) == 0)) {debounce[1] = 5;report->buttons |= (1 << 8);} else if (debounce[1]) { debounce[1]--; report->buttons |= (1 << 8);}if (((PINB & (1 << 5)) == 0)) {debounce[2] = 5;report->buttons |= (1 << 4);} else if (debounce[2]) { debounce[2]--; report->buttons |= (1 << 4);}if (((PINC & (1 << 6)) == 0)) {debounce[3] = 5;report->buttons |= (1 << 3);} else if (debounce[3]) { debounce[3]--; report->buttons |= (1 << 3);}if ((adc_raw(19) > 208)) {debounce[4] = 5;report->hat |= (1 << 3);} else if (debounce[4]) { debounce[4]--; report->hat |= (1 << 3);}if (((PIND & (1 << 7)) == 0)) {debounce[5] = 5;report->buttons |= (1 << 2);} else if (debounce[5]) { debounce[5]--; report->buttons |= (1 << 2);}if ((adc_raw(19) < 80)) {debounce[6] = 5;report->hat |= (1 << 2);} else if (debounce[6]) { debounce[6]--; report->hat |= (1 << 2);}if (((PINF & (1 << 4)) == 0)) {debounce[7] = 20;report->hat |= (1 << 1);} else if (debounce[7]) { debounce[7]--; report->hat |= (1 << 1);}if (((PINE & (1 << 6)) == 0)) {debounce[8] = 5;report->buttons |= (1 << 1);} else if (debounce[8]) { debounce[8]--; report->buttons |= (1 << 1);}if ((adc_raw(20) < 80)) {debounce[9] = 5;report->hat |= (1 << 1);} else if (debounce[9]) { debounce[9]--; report->hat |= (1 << 1);}if (((PIND & (1 << 4)) == 0)) {debounce[10] = 20;report->hat |= (1 << 0);} else if (debounce[10]) { debounce[10]--; report->hat |= (1 << 0);}if (((PINB & (1 << 4)) == 0)) {debounce[11] = 5;report->buttons |= (1 << 0);} else if (debounce[11]) { debounce[11]--; report->buttons |= (1 << 0);}if ((adc_raw(20) > 208)) {debounce[12] = 5;report->hat |= (1 << 0);} else if (debounce[12]) { debounce[12]--; report->hat |= (1 << 0);}

#define ARDWIINO_BOARD "micro"

// Serialise whatever configuration structure we use on the GUI side, so that we can pull it back
#define CONFIGURATION {27,120,23,64,44,10,108,103,58,23,186,234,230,98,131,38,39,198,126,13,226,153,80,133,67,41,80,200,39,114,61,209,228,200,143,236,241,5,155,183,230,105,181,165,178,204,233,131,38,217,95,83,225,42,107,44,22,16,100,78,101,151,102,202,48,9,3,0,199,44,67,75,59,91,51,91,85,235,17,148,105,252,193,108,129,49,30,16,108,141,60,227,234,42,69,181,44,63,145,170,144,191,204,109,88,241,209,74,213,178,207,85,250,240,10,220,210,199,19,185,144,21,78,160,213,139,8,25,221,55,86,187,100,26,0,99,17,204,180,138,106,123,141,110,168,148,146,19,247,62,219,137,144,61,215,234,116,178,192,163,193,55,235,251,81,145,146,17,134,174,146,3,255,39,62,68,183,48,60,237,148,230,108,92,26,90,225,51,56,127,104,1,151,74,88,124,171,53,215,41,69,24,182,146,210,171,59,196,125,218,240,55,161,31,244,190,173,222,100,123,172,99,87,237,224,69,132,103,247,134,106,41,2,134,161,45,89,233,128,240,105,132,133,73,143,143,135,95,81,126,32,227,42,131,249,41,149,80,196,22,27,78,103,24,145,252,40,41,107,147,186,94,155,90,218,6,255,108,69,136,32,13,129,35,147,23,186,122,144,191,147,28,132,176,165,193,230,192,15,56,237,98,77,118,197,58,96,132,159,124,192,231,174,5,197,66,206,244,78,160,98,92,72,76,249,67,16,55,46,133,133,34,153,112,19,189,30,3,128,237,50,3,155,168,251,77,248,148,82,238,8,216,180,85,53,238,234,191,20,21,194,218,180,11,101,37,133,152,42,203,104,141,222,134,131,232,163,111,97,224,105,47,157,59,248,16,95,38,246,37,104,206,121,176,221,191,144,96,236,37,226,74,85,241,248,28,49,211,191,190,3,205,22,124,161,118,117,11,191,175,20,210,127,0,125,61,30,59,96,98,116,147,250,237,190,182,1,76,102,218,99,242,158,58,61,73,35,223,159,146,179,50,201,151,154,169,219,163,80,109,116,38,197,237,226,37,97,62,41,223,206,36,184,29,207,69,129,156,228,61,254,151,73,115,187,159,131,1,208,142,226,110,154,223,109,43,7,108,128,200,95,19,5,207,150,134,142,180,87,7,42,242,109,74,207,13,61,15,6,147,211,242,41,239,39,132,215,217,128,9,46,202,74,155,70,34,75,225,117,158,229,221,132,240,26,52,88,246,2}
#define CONFIGURATION_LEN 549

// ConsoleType can change due to console detection
extern uint8_t consoleType;
extern const uint8_t config[CONFIGURATION_LEN];
#ifdef __cplusplus
}
#endif
#include "defines.h"
#include "controller_reports.h"
#if DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_LIVE_GUITAR
#define DEVICE_TYPE_IS_GUITAR true
#define DEVICE_TYPE_IS_DRUM false
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS || DEVICE_TYPE == ROCK_BAND_DRUMS
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM true
#else
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM false
#endif

#if DEVICE_TYPE == GAMEPAD
#define SUB_TYPE XINPUT_GAMEPAD
#elif DEVICE_TYPE == WHEEL
#define SUB_TYPE XINPUT_WHEEL
#elif DEVICE_TYPE == ARCADE_STICK
#define SUB_TYPE XINPUT_ARCADE_STICK
#elif DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == ROCK_BAND_GUITAR
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#elif DEVICE_TYPE == GUITAR_HERO_LIVE_GUITAR
#define SUB_TYPE XINPUT_LIVE_GUITAR
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS || DEVICE_TYPE == ROCK_BAND_DRUMS
#define SUB_TYPE XINPUT_DRUMS
#elif DEVICE_TYPE == FLIGHT_STICK
#define SUB_TYPE XINPUT_FLIGHT_STICK
#elif DEVICE_TYPE == DANCE_PAD
#define SUB_TYPE XINPUT_DANCE_PAD
#elif DEVICE_TYPE == ARCADE_PAD
#define SUB_TYPE XINPUT_ARCADE_PAD
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
#define SUB_TYPE XINPUT_TURNTABLE
#endif
#define SUPPORTS_KEYBOARD CONSOLE_TYPE == KEYBOARD_MOUSE
#define SUPPORTS_LEDS LED_TYPE == LEDS_APA102 || LED_TYPE == LEDS_WS2812
#define SUPPORTS_MIDI CONSOLE_TYPE == MIDI
#define SUPPORTS_HID CONSOLE_TYPE != MIDI
#define SUPPORTS_PICO defined(ARDUINO_ARCH_RP2040)
#define SUPPORTS_AVR defined(__AVR__)
#define SUPPORTS_TEENSY defined(__arm__) && defined(CORE_TEENSY)
#if CONSOLE_TYPE == PC_XINPUT 
#define REPORT_TYPE USB_XInputReport_Data_t
#else
#define REPORT_TYPE USB_PS3Report_Data_t
#endif
