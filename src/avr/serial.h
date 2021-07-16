#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
/** Length of the device's unique internal serial number, in bits, if present on the selected microcontroller
				 *  model.
				 */
#define INTERNAL_SERIAL_LENGTH_BITS 80

/** Start address of the internal serial number, in the appropriate address space, if present on the selected microcontroller
				 *  model.
				 */
#define INTERNAL_SERIAL_START_ADDRESS 0x0E
#define SERIAL_LEN (INTERNAL_SERIAL_LENGTH_BITS / 4)
#define SWAPENDIAN_16(x) (uint16_t)((((x)&0xFF00) >> 8) | (((x)&0x00FF) << 8))
static inline uint16_t generateSerialString(uint16_t* const UnicodeString) {
    uint8_t CurrentGlobalInt = SREG;
    cli();

    uint8_t SigReadAddress = INTERNAL_SERIAL_START_ADDRESS;

    for (uint8_t SerialCharNum = 0; SerialCharNum < (INTERNAL_SERIAL_LENGTH_BITS / 4); SerialCharNum++) {
        uint8_t SerialByte = boot_signature_byte_get(SigReadAddress);

        if (SerialCharNum & 0x01) {
            SerialByte >>= 4;
            SigReadAddress++;
        }

        SerialByte &= 0x0F;

        UnicodeString[SerialCharNum] = SWAPENDIAN_16((SerialByte >= 10) ? (('A' - 10) + SerialByte) : ('0' + SerialByte));
    }

    SREG = CurrentGlobalInt;
    return 2 + ((INTERNAL_SERIAL_LENGTH_BITS / 4) << 1);
}