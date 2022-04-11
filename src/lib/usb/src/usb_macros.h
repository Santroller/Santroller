#include <stdint.h>


#define USB_DESCRIPTOR_STRING_ARRAY(...)                                                                                                                \
    {                                                                                                                                                   \
        .bLength = sizeof(uint8_t) + sizeof(uint8_t)  + sizeof((uint16_t[]){__VA_ARGS__}), .bDescriptorType = USB_DESCRIPTOR_STRING, .UnicodeString = { __VA_ARGS__ } \
    }

#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |     \
                ((Minor & 0x0F) << 4) |     \
                (Revision & 0x0F))

#define USB_CONFIG_POWER_MA(mA) ((mA) >> 1)


#define NO_DESCRIPTOR 0

#define LANGUAGE_ID_ENG 0x0409


#define USB_CONFIG_ATTRIBUTE_RESERVED 0x80
#define USB_CONFIG_ATTRIBUTE_SELFPOWERED 0x40
#define USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP 0x20

#define HID_DESCRIPTOR_REPORT 0x22

#define ENDPOINT_TATTR_NO_SYNC (0 << 2)
#define ENDPOINT_TATTR_ASYNC (1 << 2)
#define ENDPOINT_TATTR_ADAPTIVE (2 << 2)
#define ENDPOINT_TATTR_SYNC (3 << 2)
#define ENDPOINT_USAGE_DATA (0 << 4)
#define ENDPOINT_USAGE_FEEDBACK (1 << 4)
#define ENDPOINT_USAGE_IMPLICIT_FEEDBACK (2 << 4)
