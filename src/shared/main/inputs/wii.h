#ifdef INPUT_WII
    uint8_t *wiiData;
    bool wiiValid = wiiDataValid();
    lastWiiWasSuccessful = wiiValid;
    // If we didn't send the last packet, then we need to wait some time as the wii controllers do not like being polled quickly
    if (micros() - lastTick > 750) {
        wiiData = tickWii();
        lastTick = micros();
        if (wiiValid) {
            memset(&temp_report, 0, sizeof(temp_report));
            wii_to_universal_report(wiiData, wiiBytes, wiiControllerType, hiRes, &temp_report);
        }
    }
#endif