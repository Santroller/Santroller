#ifdef INPUT_CRKD_NECK
    lastCrkdWasSuccessful = read_uart(CRKD_UART_PORT, 0xA5, 11, (uint8_t*)&lastCrkd);
#endif