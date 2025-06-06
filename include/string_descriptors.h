#include <stdint.h>

#include "progmem.h"
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

#define __CAT(x, y) x##y
#define CAT(x, y) __CAT(x, y)

// Strings have a null terminator. We don't want that, but bLength and bDescriptorType take two extra bytes so that cuts the null terminator from the string.
#define USB_DESCRIPTOR_STRING_UTF(s)                                        \
    {                                                                       \
        .bLength = (sizeof(CAT(u, s))), \
        .bDescriptorType = USB_DESCRIPTOR_STRING,                           \
        .UnicodeString = { CAT(u, s) }                                      \
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
                               */
}
__attribute__((packed)) STRING_DESCRIPTOR;

extern const PROGMEM STRING_DESCRIPTOR *const descriptorStrings[3];
extern const PROGMEM STRING_DESCRIPTOR *const ps3DescriptorStrings[3];
extern const PROGMEM STRING_DESCRIPTOR xboxString;
extern const PROGMEM STRING_DESCRIPTOR languageString;
extern const PROGMEM STRING_DESCRIPTOR manufacturerString;
extern const PROGMEM STRING_DESCRIPTOR productString;
extern const PROGMEM STRING_DESCRIPTOR rtString;
#ifdef __cplusplus
}
#endif
