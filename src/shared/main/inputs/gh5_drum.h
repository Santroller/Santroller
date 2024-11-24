#ifdef INPUT_GH5_DRUM
    uint8_t *fivetar_buttons = lastSuccessfulGH5Packet;
    bool gh5Valid = twi_readFromPointer(GH5_DRUM_TWI_PORT, GH5DRUM_ADDR, GH5DRUM_BUTTONS_PTR, sizeof(lastSuccessfulGH5Packet), lastSuccessfulGH5Packet);
    lastGH5WasSuccessful = gh5Valid;
    if (lastSuccessfulGH5Packet[1] == 0x99) {
        switch (lastSuccessfulGH5Packet[2]) {
            case MIDI_DRUM_KICK:
                drumVelocity[DRUM_KICK] = lastSuccessfulGH5Packet[3];
                break;
            case MIDI_DRUM_GREEN:
                drumVelocity[DRUM_GREEN] = lastSuccessfulGH5Packet[3];
                break;
            case MIDI_DRUM_RED:
                drumVelocity[DRUM_RED] = lastSuccessfulGH5Packet[3];
                break;
            case MIDI_DRUM_YELLOW:
                drumVelocity[DRUM_YELLOW] = lastSuccessfulGH5Packet[3];
                break;
            case MIDI_DRUM_BLUE:
                drumVelocity[DRUM_BLUE] = lastSuccessfulGH5Packet[3];
                break;
            case MIDI_DRUM_ORANGE:
                drumVelocity[DRUM_ORANGE] = lastSuccessfulGH5Packet[3];
                break;
            case MIDI_DRUM_HI_HAT:
                drumVelocity[DRUM_HIHAT] = lastSuccessfulGH5Packet[3];
                break;
        }
    }
#endif

// maps perfectly to the MIDI_DRUM_* defines in defines.h
// band hero:
// its i2c (like gh5 and turntable)
// its less useful, as we can just hardwire the drums if we are replacing the brain
// the only useful thing we gain is midi
// addr: 0x0D
// ptr: 0x10

// blue:
// 10 99 30 57

// green:
// 10 99 2D 1C
// red:
// 10 99 26 19

// yellow:
// 10 99 2E 3C

// orange:
// 10 99 31 7A

// kick:
// 10 99 24 42

// hi hat:
// 10 99 24 1C
// 10 99 64 67