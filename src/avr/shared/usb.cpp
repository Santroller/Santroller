#include <stdint.h>
// TODO: implement all this using USB HOST LIB (TBH probably not worth it)
void send_report_to_controller(uint8_t deviceType, uint8_t *report, uint8_t len) {
}
uint8_t get_device_address_for(uint8_t deviceType) {
    return 0;
}
uint8_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer) {
    return 0;
}