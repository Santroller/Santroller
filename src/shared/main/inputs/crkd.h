#ifdef INPUT_CRKD_NECK
lastCrkdWasSuccessful = read_uart(CRKD_UART_PORT, 0xA5, 11, (uint8_t *)&lastCrkd);
if (millis() - last_poll_crkd >= 10)
{
    send_uart(CRKD_UART_PORT, crkd_magic_data, sizeof(crkd_magic_data));
    last_poll_crkd = millis();
}
#endif