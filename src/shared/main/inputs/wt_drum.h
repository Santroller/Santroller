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

// WT:
// This one is most interesting as its over the brain connector
// SPI:
// clock polarity: 1
// clock phase: 0
// chip select: active low
// msb-first

// send AA 55, then zeros. need to test if we can just always send aa 55 00 00 00
// kick:
// AA 05 99 24 21

// blue:
// AA 01 99 30 20

// green:
// AA 01 99 2D 25

// hi hat:
// AA 01 99 64 00
// AA 01 99 64 7F