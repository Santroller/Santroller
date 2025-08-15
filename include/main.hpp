#include "usb/usb_descriptors.h"
#include "events.pb.h"
void send_event(proto_Event event, bool resend_events);
void send_debug(uint8_t* data, size_t len);