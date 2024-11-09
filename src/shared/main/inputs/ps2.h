#ifdef INPUT_PS2
    uint8_t *ps2Data = tickPS2();
    bool ps2Valid = ps2Data != NULL;
    lastPS2WasSuccessful = ps2Valid;
    if (ps2Valid) {
        memset(&lastSuccessfulPS2Packet, 0, sizeof(lastSuccessfulPS2Packet));
        ps2_to_universal_report(ps2Data, sizeof(lastSuccessfulPS2Packet), ps2ControllerType, &lastSuccessfulPS2Packet);
    }
#endif