#include "midi.h"
#include "output/descriptors.h"
#include "config/eeprom.h"
#include "output/controller_structs.h"
#include "input/input_handler.h"

uint8_t lastmidi[XBOX_BTN_COUNT + XBOX_AXIS_COUNT];
void fillMIDIReport(void *ReportData, uint16_t *const ReportSize,
                        Controller_t *controller) {
  USB_MIDI_Data_t *data = ReportData;
  uint8_t idx = 0;
  for (int i = 0; i < XBOX_BTN_COUNT + XBOX_AXIS_COUNT; i++) {
    if (config.midi.midiType[i] != DISABLED) {
      // Channel 10(percussion)
      uint8_t channel = config.midi.channel[i];
      uint8_t midipitch = config.midi.note[i];
      uint8_t midicommand = config.midi.midiType[i] == NOTE
                                ? MIDI_COMMAND_NOTE_ON
                                : MIDI_COMMAND_CONTROL_CHANGE;
      uint8_t vel = getVelocity(controller, i) >> 1;
      if (lastmidi[i] == vel) continue;
      lastmidi[i] = vel;
      data->midi[idx].Event = MIDI_EVENT(0, midicommand);
      data->midi[idx].Data1 = midicommand | channel;
      data->midi[idx].Data2 = midipitch;
      data->midi[idx].Data3 = vel;
      idx++;
    }
  }

  *ReportSize = idx * sizeof(MIDI_EventPacket_t);
}