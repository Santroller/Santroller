#ifdef INPUT_WII
    uint8_t *wiiData;
    // If we didn't send the last packet, then we need to wait some time as the wii controllers do not like being polled quickly
    if (micros() - lastTick > 750) {
        lastTick = micros();
        wiiData = tickWii();
    } else {
        wiiData = lastSuccessfulWiiPacket;
    }
    bool wiiValid = wiiDataValid();
    lastWiiWasSuccessful = wiiValid;
    uint8_t wiiButtonsLow, wiiButtonsHigh, vel, which, lastTapWiiGh5, lastTapWii = 0;
    uint16_t accX, accY, accZ = 0;
    if (wiiValid) {
        memcpy(lastSuccessfulWiiPacket, wiiData, sizeof(lastSuccessfulWiiPacket));
        wiiButtonsLow = ~wiiData[4];
        wiiButtonsHigh = ~wiiData[5];
        if (hiRes) {
            wiiButtonsLow = ~wiiData[6];
            wiiButtonsHigh = ~wiiData[7];
        }
#ifdef INPUT_WII_TAP
        lastTapWii = (wiiData[2] & 0x1f);

        // GH3 guitars set this bit, while WT and GH5 guitars do not
        if (!hasTapBar) {
            if (lastTapWii == 0x0F) {
                hasTapBar = true;
            }
            lastTapWii = 0;
            lastTapWiiGh5 = 0;
        } else if (lastTapWii == 0x0f) {
            lastTapWiiGh5 = 0;
        } else if (lastTapWii < 0x05) {
            lastTapWiiGh5 = 0x95;
        } else if (lastTapWii < 0x0B) {
            lastTapWiiGh5 = 0xB0;
        } else if (lastTapWii < 0x0F) {
            lastTapWiiGh5 = 0xCD;
        } else if (lastTapWii < 0x12) {
            lastTapWiiGh5 = 0xE6;
        } else if (lastTapWii < 0x14) {
            lastTapWiiGh5 = 0x1A;
        } else if (lastTapWii < 0x17) {
            lastTapWiiGh5 = 0x2F;
        } else if (lastTapWii < 1A) {
            lastTapWiiGh5 = 0x49;
        } else if (lastTapWii < 0x1F) {
            lastTapWiiGh5 = 0x66;
        } else {
            lastTapWiiGh5 = 0x7F;
        }

#endif
        // Turntable bits are all split up, so we slice them back up
        ltt_t.ltt5  = (wiiData[4] & 1);
        ltt_t.ltt40 = (wiiData[3] & 0x1F);
        rtt_t.rtt0  = (wiiData[2] & 0x80) >> 7;
        rtt_t.rtt21 = (wiiData[1] & 0xC0) >> 6;
        rtt_t.rtt43 = (wiiData[0] & 0xC0) >> 6;
        rtt_t.rtt5  = (wiiData[2] & 1);
#ifdef INPUT_WII_DRUM
        vel = (7 - (wiiData[3] >> 5)) << 5;
        which = (wiiData[2] & 0b00111110) >> 1;
        switch (which) {
            case 0x1B:
                drumVelocity[DRUM_KICK] = vel;
                break;
            case 0x12:
                drumVelocity[DRUM_GREEN] = vel;
                break;
            case 0x19:
                drumVelocity[DRUM_RED] = vel;
                break;
            case 0x11:
                drumVelocity[DRUM_YELLOW] = vel;
                break;
            case 0x0F:
                drumVelocity[DRUM_BLUE] = vel;
                break;
            case 0x0E:
                drumVelocity[DRUM_ORANGE] = vel;
                break;
        }
        // We only get velocity on events above, so zero them when the digital input is off
        if (!((wiiButtonsHigh) & (1 << 4))) {
            drumVelocity[DRUM_GREEN] = 0;
        }
        if (!((wiiButtonsHigh) & (1 << 6))) {
            drumVelocity[DRUM_RED] = 0;
        }
        if (!((wiiButtonsHigh) & (1 << 5))) {
            drumVelocity[DRUM_YELLOW] = 0;
        }
        if (!((wiiButtonsHigh) & (1 << 3))) {
            drumVelocity[DRUM_BLUE] = 0;
        }
        if (!((wiiButtonsHigh) & (1 << 7))) {
            drumVelocity[DRUM_ORANGE] = 0;
        }
        if (!((wiiButtonsHigh) & (1 << 2))) {
            drumVelocity[DRUM_KICK] = 0;
        }
#endif
#ifdef INPUT_WII_NUNCHUK
        accX = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
        accY = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
        accZ = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
#endif
    }
#endif