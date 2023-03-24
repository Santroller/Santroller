#include "progmem.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define LANGUAGE_ID_ENG 0x0409
#define USB_DESCRIPTOR_STRING 0x03

#define USB_DESCRIPTOR_STRING_ARRAY(...)                                                    \
    {                                                                                       \
        .bLength = (sizeof(uint8_t) + sizeof(uint8_t) + sizeof((uint16_t[]){__VA_ARGS__})), \
        .bDescriptorType = USB_DESCRIPTOR_STRING,                                           \
        .UnicodeString = { __VA_ARGS__ }                                                    \
    }
typedef struct
{
    uint8_t bLength;          // Length of this descriptor.
    uint8_t bDescriptorType;  // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint16_t UnicodeString[]; /**< String data, as unicode characters (alternatively,
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
} __attribute__((packed)) STRING_DESCRIPTOR;

extern const PROGMEM STRING_DESCRIPTOR *const descriptorStrings[3];
extern const PROGMEM STRING_DESCRIPTOR *const ps3DescriptorStrings[3];
extern const PROGMEM STRING_DESCRIPTOR xboxString;
extern const PROGMEM STRING_DESCRIPTOR languageString;
extern const PROGMEM STRING_DESCRIPTOR manufacturerString;
extern const PROGMEM STRING_DESCRIPTOR productString;
#ifdef __cplusplus
}
#endif
