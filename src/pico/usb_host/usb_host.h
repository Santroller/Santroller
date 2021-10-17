
#include <stdint.h>
#include <tusb.h>
void init_usb_host(void);
void tick_usb_host(void);
void send_control_request(uint8_t address, uint8_t endpoint, const tusb_control_request_t request, bool terminateEarly, uint8_t* d);
TUSB_Descriptor_Device_t getHostDescriptor(void);