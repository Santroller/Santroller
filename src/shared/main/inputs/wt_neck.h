#ifdef INPUT_WT_SLAVE_NECK
    rawWtPeripheral = slaveReadWt();
#endif
#ifdef INPUT_WT_NECK
    rawWt = tickWt();
#endif