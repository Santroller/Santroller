#include "reports.h"
#include "descriptors.h"
#include "input/input_handler.h"
// #include "reports/keyboard.h"
// #include "reports/midi.h"
#include "reports/mouse.h"
#include "reports/ps3.h"
#include "reports/xinput.h"
#include "util/util.h"

void (*fillReport)(void *ReportData, uint8_t *const ReportSize,
                   Controller_t *controller) = NULL;

void initReports(Configuration_t* config) {
  if (fullDeviceType == MOUSE) {
    fillReport = fillMouseReport;
  } else if (fullDeviceType >= MIDI_GAMEPAD) {
    // initMIDI(config);
    // fillReport = fillMIDIReport;
  } else if (fullDeviceType <= XINPUT_ARCADE_PAD) {
    fillReport = fillXInputReport;
  } else if (fullDeviceType >= KEYBOARD_GAMEPAD &&
             fullDeviceType <= KEYBOARD_ROCK_BAND_DRUMS) {
    // initKeyboard(config);
    // fillReport = fillKeyboardReport;
  } else {
    initPS3();
    fillReport = fillPS3Report;
  }
}