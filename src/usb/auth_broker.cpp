#include "usb/auth_broker.h"
#include <memory>

class UsbHostInterface;

// Global instance
AuthBroker auth_broker;

void AuthBroker::register_handler(ConsoleMode mode, AuthHandler handler) {
    handlers[mode] = handler;
}

void AuthBroker::unregister_handler(ConsoleMode mode) {
    handlers.erase(mode);
}

bool AuthBroker::forward_auth(ConsoleMode mode, XGIPProtocol* packet) {
    auto it = handlers.find(mode);
    if (it != handlers.end() && it->second) {
        it->second(packet);
        return true;
    }
    return false;
}

bool AuthBroker::has_handler(ConsoleMode mode) const {
    return handlers.find(mode) != handlers.end();
}

void AuthBroker::register_auth_device(ConsoleMode mode, std::shared_ptr<UsbHostInterface> device) {
    auth_devices[mode] = device;
}

std::shared_ptr<UsbHostInterface> AuthBroker::get_auth_device(ConsoleMode mode) const {
    auto it = auth_devices.find(mode);
    return (it != auth_devices.end()) ? it->second : nullptr;
}

void AuthBroker::unregister_auth_device(ConsoleMode mode) {
    auth_devices.erase(mode);
}
