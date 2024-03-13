#ifdef INPUT_MPR121
uint16_t mpr121_touch = tick_mpr121_cap();
#if MPR121_ENABLE
uint8_t mpr121_gpio = tick_mpr121_gpio();
#endif
#endif