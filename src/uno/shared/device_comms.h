#pragma once
#define BAUD 1000000
#define FRAME_START_DEVICE 0x7c
#define FRAME_START_SERIAL 0x7e
#define FRAME_END 0x7f
#define ESC 0x7b

/** Macro for calculating the baud value from a given baud rate when the \c U2X
 * (double speed) bit is set.
 *
 *  \param[in] Baud  Target serial UART baud rate.
 *
 *  \return Closest UBRR register value for the given UART frequency.
 */
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)