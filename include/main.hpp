#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
void send_debug(uint8_t* data, size_t len);
void initDebug();
void deinitDebug();
void reinitialize_device_stack();

bool mode_recently_changed();