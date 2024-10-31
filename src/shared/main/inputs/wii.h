#ifdef INPUT_WII
    uint8_t *wiiData;
    // If we didn't send the last packet, then we need to wait some time as the wii controllers do not like being polled quickly
    if (micros() - lastTick > 750) {
        wiiData = tickWii();
        lastTick = micros();
    } else {
        wiiData = lastSuccessfulWiiPacket;
    }
    bool wiiValid = wiiDataValid();
    lastWiiWasSuccessful = wiiValid;
    if (wiiValid) {
        memcpy(lastSuccessfulWiiPacket, wiiData, sizeof(lastSuccessfulWiiPacket));
    }
#endif