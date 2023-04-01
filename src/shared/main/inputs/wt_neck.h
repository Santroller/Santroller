#ifdef INPUT_WT_NECK
    rawWt = checkWt(4) | (checkWt(3) << 1) | (checkWt(2) << 2) | (checkWt(0) << 3) | (checkWt(1) << 4);
#endif