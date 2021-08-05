
#define BAUD 1000000
#define FRAME_START_FEATURE_READ 0x7d
#define FRAME_START_FEATURE_WRITE 0x7e
#define FRAME_START_WRITE 0x78
#define FRAME_DONE 0x77

#define USART2USB_BUFLEN 128 // 0xFF - 8bit
#define USB2USART_BUFLEN 64 // 0x7F - 7bit

/** Macro for calculating the baud value from a given baud rate when the \c U2X
 * (double speed) bit is not set.
 *
 *  \param[in] Baud  Target serial UART baud rate.
 *
 *  \return Closest UBRR register value for the given UART frequency.
 */
#define SERIAL_UBBRVAL(Baud) ((((F_CPU / 16) + (Baud / 2)) / (Baud)) - 1)

/** Macro for calculating the baud value from a given baud rate when the \c U2X
 * (double speed) bit is set.
 *
 *  \param[in] Baud  Target serial UART baud rate.
 *
 *  \return Closest UBRR register value for the given UART frequency.
 */
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)