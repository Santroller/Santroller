#ifdef INPUT_WT_NECK
    rawWt = checkWt(1) | (checkWt(0) << 1) | (checkWt(2) << 2) | (checkWt(3) << 3) | (checkWt(4) << 4);
#endif

#ifdef INPUT_WT_SLAVE_NECK
    rawWtPeripheral = slaveReadWt();
#endif