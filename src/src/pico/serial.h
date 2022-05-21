#include <pico/unique_id.h>
#include <stdint.h>
#include <Usb.h>
#define SERIAL_LEN (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1)
static inline uint16_t generateSerialString(uint16_t* const UnicodeString) {
    char id[SERIAL_LEN];
    pico_get_unique_board_id_string(id, sizeof(id));
    for (int i = 0; i < sizeof(id); i++) {
        UnicodeString[i] = id[i];
    }
    return sizeof(id);
}

typedef struct
{
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint16_t UnicodeString[PICO_UNIQUE_BOARD_ID_SIZE_BYTES];       /**< String data, as unicode characters (alternatively,
				                           *   string language IDs). If normal ASCII characters are
				                           *   to be used, they must be added as an array of characters
				                           *   rather than a normal C string so that they are widened to
				                           *   Unicode size.
				                           *
				                           *   Under GCC, strings prefixed with the "L" character (before
				                           *   the opening string quotation mark) are considered to be
				                           *   Unicode strings, and may be used instead of an explicit
				                           *   array of ASCII characters on little endian devices with
				                           *   UTF-16-LE \c wchar_t encoding.
				                           */
} __attribute__((packed)) STRING_DESCRIPTOR_SERIAL;

STRING_DESCRIPTOR_SERIAL serialString = {
    bLength: sizeof(STRING_DESCRIPTOR_SERIAL),
    bDescriptorType: USB_DESCRIPTOR_STRING,
    UnicodeString : {}
};