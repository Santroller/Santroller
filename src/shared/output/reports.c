#include "reports.h"
#include "descriptors.h"
#include "input/input_handler.h"
#include "util/util.h"
#include "reports/xinput.h"
#include "reports/ps3.h"
#include "reports/keyboard.h"
#include "reports/midi.h"

void (*fillReport)(void *ReportData, uint16_t *const ReportSize,
                   Controller_t *controller) = NULL;

void initReports(void) {
  if (config.main.subType >= MIDI_CONTROLLER) {
    fillReport = fillMIDIReport;
  } else if (config.main.subType <= XINPUT_ARCADE_PAD) {
    fillReport = fillXInputReport;
  } else if (config.main.subType == KEYBOARD) {
    initKeyboard();
    fillReport = fillKeyboardReport;
  } else {
    initPS3();
    fillReport = fillPS3Report;
  }
}