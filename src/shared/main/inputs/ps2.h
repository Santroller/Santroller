#ifdef INPUT_PS2
    uint8_t *ps2Data = tickPS2();
    bool ps2Valid = ps2Data != NULL;
    lastPS2WasSuccessful = ps2Valid;
    uint8_t lastTapPS2, lastTapPS2GH5 = 0x80;
    if (ps2Valid) {
        memcpy(lastSuccessfulPS2Packet, ps2Data, sizeof(lastSuccessfulPS2Packet));
        lastTapPS2 = ps2Data[7];
        if (lastTapPS2 > 0x75  && lastTapPS2 < 0x85) {
            lastTapPS2GH5 = 0;
        } else if (lastTapPS2 < 0x1F) {
            lastTapPS2GH5 = 0x95;
        } else if (lastTapPS2 < 0x3F) {
            lastTapPS2GH5 = 0xB0;
        } else if (lastTapPS2 < 0x5F) {
            lastTapPS2GH5 = 0xCD;
        } else if (lastTapPS2 < 0x6F) {
            lastTapPS2GH5 = 0xE6;
        } else if (lastTapPS2 < 0x9F) {
            lastTapPS2GH5 = 0x1A;
        } else if (lastTapPS2 < 0xAF) {
            lastTapPS2GH5 = 0x2F;
        } else if (lastTapPS2 < 0xCF) {
            lastTapPS2GH5 = 0x49;
        } else if (lastTapPS2 < 0xEF) {
            lastTapPS2GH5 = 0x66;
        } else {
            lastTapPS2GH5 = 0x7F;
        }
    }
    
#endif