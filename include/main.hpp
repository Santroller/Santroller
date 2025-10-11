#include "usb/usb_descriptors.h"
#include "events.pb.h"
void send_event(proto_Event event);
void send_debug(uint8_t* data, size_t len);
bool tool_closed();
extern bool reinit;
extern bool seenPs4;
extern bool seenWindowsXb1;
extern bool seenOsDescriptorRead;
extern bool seenReadAnyDeviceString;
extern bool seenHidDescriptorRead;