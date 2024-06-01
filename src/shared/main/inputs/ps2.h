#ifdef INPUT_PS2
    uint8_t *ps2Data = tickPS2();
    bool ps2Valid = ps2Data != NULL;
    lastPS2WasSuccessful = ps2Valid;
    uint8_t lastTapPS2, lastTapPS2GH5 = 0x80;
    if (ps2Valid) {
        memcpy(lastSuccessfulPS2Packet, ps2Data, sizeof(lastSuccessfulPS2Packet));
        lastTapPS2 = ps2Data[7];
        if (lastTapPS2 < 0x2F) {
            lastTapPS2GH5 = 0x15;
        } else if (lastTapPS2 < 0x3F) {
            lastTapPS2GH5 = 0x30;
        } else if (lastTapPS2 < 0x5F) {
            lastTapPS2GH5 = 0x4D;
        } else if (lastTapPS2 < 0x6F) {
            lastTapPS2GH5 = 0x66;
        } else if (lastTapPS2 < 0x8F) {
            lastTapPS2GH5 = 0x80;
        } else if (lastTapPS2 < 0x9F) {
            lastTapPS2GH5 = 0x9A;
        } else if (lastTapPS2 < 0xAF) {
            lastTapPS2GH5 = 0xAF;
        } else if (lastTapPS2 < 0xCF) {
            lastTapPS2GH5 = 0xC9;
        } else if (lastTapPS2 < 0xEF) {
            lastTapPS2GH5 = 0xE6;
        } else {
            lastTapPS2GH5 = 0xFF;
        }
    }
    
#endif