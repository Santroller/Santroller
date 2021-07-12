#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define SERIAL_LEN (INTERNAL_SERIAL_LENGTH_BITS / 4)
static inline uint16_t generateSerialString(uint16_t* const UnicodeString) {
    uint_t CurrentGlobalInt = SREG;
    cli();

    uint8_t SigReadAddress = INTERNAL_SERIAL_START_ADDRESS;

    for (uint8_t SerialCharNum = 0; SerialCharNum < (INTERNAL_SERIAL_LENGTH_BITS / 4); SerialCharNum++) {
        uint8_t SerialByte = boot_signature_byte_get(SigReadAddress);

        if (SerialCharNum & 0x01) {
            SerialByte >>= 4;
            SigReadAddress++;
        }

        SerialByte &= 0x0F;

        UnicodeString[SerialCharNum] = cpu_to_le16((SerialByte >= 10) ? (('A' - 10) + SerialByte) : ('0' + SerialByte));
    }

    SREG = CurrentGlobalInt;
    return USB_STRING_LEN(INTERNAL_SERIAL_LENGTH_BITS / 4);
}