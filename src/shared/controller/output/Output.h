#include "../Controller.h"
class Output {
    virtual void init() = 0;
    virtual void update(Controller controller) = 0;
    virtual void usb_connect() = 0;
    virtual void usb_disconnect() = 0;
    virtual void usb_configuration_changed() = 0;
    virtual void usb_control_request() = 0;
    virtual void usb_start_of_frame() = 0;
};