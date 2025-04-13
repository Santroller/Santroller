#pragma once
#define GH_MIDI_NOTE_KICK 36
#define GH_MIDI_NOTE_RED 38
#define GH_MIDI_NOTE_GREEN 45
#define GH_MIDI_NOTE_YELLOW 46
#define GH_MIDI_NOTE_BLUE 48
#define GH_MIDI_NOTE_ORANGE 49
#define GH_MIDI_NOTE_KICK2 100

#define RB_MIDI_NOTE_KICK 36
#define RB_MIDI_NOTE_RED 38
#define RB_MIDI_NOTE_GREEN 41
#define RB_MIDI_NOTE_YELLOW 48
#define RB_MIDI_NOTE_BLUE 45
#define RB_MIDI_NOTE_GREEN_CYMBAL 49
#define RB_MIDI_NOTE_YELLOW_CYMBAL 22
#define RB_MIDI_NOTE_BLUE_CYMBAL 51
#define RB_MIDI_NOTE_KICK2 44
#include "state/base.hpp"
class DrumParser {
   public:
    template <typename T>
    static void updatePadsRB(T* report, san_base_t* data, bool up, bool down, uint8_t greenVelocity, uint8_t redVelocity, uint8_t yellowVelocity, uint8_t blueVelocity) {
        bool green = report->a;
        bool red = report->b;
        bool yellow = report->y;
        bool blue = report->x;
        bool pad = report->padFlag;
        bool cymbal = report->cymbalFlag;
        if (!red) {
            data->midi.midiVelocities[RB_MIDI_NOTE_RED] = 0;
        }
        if (!green) {
            data->midi.midiVelocities[RB_MIDI_NOTE_GREEN] = 0;
            data->midi.midiVelocities[RB_MIDI_NOTE_GREEN_CYMBAL] = 0;
        }
        if (!yellow) {
            data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW] = 0;
            data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW_CYMBAL] = 0;
        }
        if (!blue) {
            data->midi.midiVelocities[RB_MIDI_NOTE_BLUE] = 0;
            data->midi.midiVelocities[RB_MIDI_NOTE_BLUE_CYMBAL] = 0;
        } /* If you hit both the pad and cymbal of the same colour simultaneously, the velocity for the cymbal          \
             is placed in the redVelocity, but the red button won't be pressed. */
        if (!red && redVelocity) {
            if (up) {
                data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW] = yellowVelocity >> 1;
                data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW_CYMBAL] = redVelocity >> 1;
            } else if (down) {
                data->midi.midiVelocities[RB_MIDI_NOTE_BLUE] = blueVelocity >> 1;
                data->midi.midiVelocities[RB_MIDI_NOTE_BLUE_CYMBAL] = redVelocity >> 1;
            } else {
                data->midi.midiVelocities[RB_MIDI_NOTE_GREEN] = greenVelocity >> 1;
                data->midi.midiVelocities[RB_MIDI_NOTE_GREEN_CYMBAL] = redVelocity >> 1;
            }
            pad = false;
            cymbal = false;
        }
        if (pad && cymbal) { /* The d-pad inputs let us resolve the ambiguity of a pad+cymbal hit                                      \
                                Only d-pad is checked here since it is the only unique identifier due to hardware bugs                 \
                                (sometimes the color flag is released before the d-pad input is, particularly at high polling rates)*/

            /* Yellow */
            if (up) {
                data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW_CYMBAL] = yellowVelocity >> 1;
                yellow = false;
                cymbal = false;
            }

            /* Blue */
            if (down) {
                data->midi.midiVelocities[RB_MIDI_NOTE_BLUE_CYMBAL] = blueVelocity >> 1;
                blue = false;
                cymbal = false;
            }

            /* Green */
            if (!(up || down)) {
                data->midi.midiVelocities[RB_MIDI_NOTE_GREEN_CYMBAL] = greenVelocity >> 1;
                green = false;
                cymbal = false;
            }
        }
        if (pad) {
            if (yellow) {
                data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW] = yellowVelocity >> 1;
            }
            if (blue) {
                data->midi.midiVelocities[RB_MIDI_NOTE_BLUE] = blueVelocity >> 1;
            }
            if (red) {
                data->midi.midiVelocities[RB_MIDI_NOTE_RED] = redVelocity >> 1;
            }
            if (green) {
                data->midi.midiVelocities[RB_MIDI_NOTE_GREEN] = greenVelocity >> 1;
            }
        }

        if (cymbal) {
            if (blue) {
                data->midi.midiVelocities[RB_MIDI_NOTE_BLUE_CYMBAL] = yellowVelocity >> 1;
            }
            if (yellow) {
                data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW_CYMBAL] = blueVelocity >> 1;
            }
            if (green) {
                data->midi.midiVelocities[RB_MIDI_NOTE_GREEN_CYMBAL] = greenVelocity >> 1;
            }
        }
        data->midi.midiVelocities[RB_MIDI_NOTE_KICK] = report->kick1 ? 0x7F : 0x00;
        data->midi.midiVelocities[RB_MIDI_NOTE_KICK2] = report->kick2 ? 0x7F : 0x00;
    }
};