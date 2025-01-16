#include <stdint.h>
#include "defines.h"
#if USB_HOST_STACK
void send_report_to_controller(uint8_t deviceType, const uint8_t *report, uint8_t len) {
}
USB_Device_Type_t get_device_address_for(uint8_t deviceType) {
    return 0;
}
uint32_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer, bool* status) {
    return 0;
}
#endif
void authentication_successful() {
}