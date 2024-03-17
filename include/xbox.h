#include <stdint.h>
#include "wcid.h"
#include "reports/xinput_reports.h"
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

extern const XInputVibrationCapabilities_t capabilities1;
extern const XInputInputCapabilities_t capabilities2;
extern const OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR ExtendedIDs;
extern const OS_COMPATIBLE_ID_DESCRIPTOR DevCompatIDs;  
extern const OS_COMPATIBLE_ID_DESCRIPTOR DevCompatIDs360;
extern const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsPS3;
extern const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsOne; 
extern const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsOneDetect;