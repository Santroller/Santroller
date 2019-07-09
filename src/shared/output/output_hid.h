#include "output_handler.h"
#include "usb/Descriptors.h"

void hid_init(event_pointers*);
void hid_tick(controller_t);

extern controller_t last_controller;
extern USB_ClassInfo_HID_Device_t interface;