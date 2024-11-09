#ifdef INPUT_PS2
    uint8_t *ps2Data = tickPS2();
    bool ps2Valid = ps2Data != NULL;
    lastPS2WasSuccessful = ps2Valid;
    if (ps2Valid) {
        memcpy(lastSuccessfulPS2Packet, ps2Data, sizeof(lastSuccessfulPS2Packet));
        ps2_to_universal_report(ps2Data, sizeof(lastSuccessfulPS2Packet), ps2ControllerType, &ps2_report);
    }
#endif