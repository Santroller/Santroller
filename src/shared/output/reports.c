#include "reports.h"
#include "descriptors.h"
#include "input/input_handler.h"
#include "reports/keyboard.h"
#include "reports/midi.h"
#include "reports/ps3.h"
#include "reports/xinput.h"
#include "reports/mouse.h"
#include "util/util.h"

void (*fillReport)(void *ReportData, uint16_t *const ReportSize,
                   Controller_t *controller) = NULL;

void initReports(void) {
  if (config.main.subType == MOUSE) {
    fillReport = fillMouseReport;
  } else if (config.main.subType >= MIDI_GAMEPAD) {
    fillReport = fillMIDIReport;
  } else if (config.main.subType <= XINPUT_ARCADE_PAD) {
    fillReport = fillXInputReport;
  } else if (config.main.subType >= KEYBOARD_GAMEPAD &&
             config.main.subType <= KEYBOARD_ROCK_BAND_DRUMS) {
    initKeyboard();
    fillReport = fillKeyboardReport;
  } else {
    initPS3();
    fillReport = fillPS3Report;
  }
}