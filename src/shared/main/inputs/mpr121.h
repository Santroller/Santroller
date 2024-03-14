#ifdef INPUT_MPR121
    uint16_t mpr121_raw = tick_mpr121();
    if (mpr121_init) {
        lastMpr121 = mpr121_raw;
    }
#endif