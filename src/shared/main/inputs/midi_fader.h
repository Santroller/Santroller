#ifdef INPUT_MIDI_FADER
lastMidiFaderWasSuccessful = read_uart(MIDI_FADER_UART_PORT, 1, (uint8_t *)&lastMidiFader);
#endif