#include <stdint.h>
typedef struct {
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint8_t reserved[2];
    uint8_t subtype;
    uint8_t reserved2;
    uint8_t bEndpointAddressIn;
    uint8_t bMaxDataSizeIn;
    uint8_t reserved3[5];
    uint8_t bEndpointAddressOut;
    uint8_t bMaxDataSizeOut;
    uint8_t reserved4[2];
} __attribute__((packed)) XBOX_ID_DESCRIPTOR;