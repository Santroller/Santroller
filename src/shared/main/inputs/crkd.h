#ifdef INPUT_CRKD_NECK
lastCrkdWasSuccessful = read_uart(CRKD_UART_PORT, 0xA5, 11, (uint8_t *)&lastCrkd);
if (lastCrkdWasSuccessful && millis() - lastCrkdSend > 10)
    {
        // kick the neck over to polling at 1ms
        uint8_t data[] = {0xA5, 0xC1, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5};
        send_uart(CRKD_UART_PORT, data, sizeof(data));
        lastCrkdSend = millis();
    }
#endif