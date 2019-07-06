#include "output_handler.h"
enum InterfaceDescriptors_t { INTERFACE_ID_HID = 0 };
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_GamepadHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;
#define HID_EPADDR (ENDPOINT_DIR_IN | 1)
#define HID_EPSIZE 64
#define HID_REPORTSIZE 64

void hid_init(event_pointers*);
void hid_tick(controller_t);