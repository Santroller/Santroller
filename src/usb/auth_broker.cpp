#include "usb/auth_broker.h"
#include <memory>

class UsbHostInterface;

// Global instance
AuthBroker auth_broker;

void AuthBroker::register_handler(ConsoleMode mode, AuthHandler handler) {
    unregister_handler(mode);
    handlers[mode] = std::move(handler);
}

void AuthBroker::unregister_handler(ConsoleMode mode) {
    auto it = handlers.find(mode);
    if (it != handlers.end()) {
        auto handler = std::move(it->second);
        handlers.erase(it);
    }
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
    unregister_auth_device(mode);
    auth_devices[mode] = std::move(device);
}

std::shared_ptr<UsbHostInterface> AuthBroker::get_auth_device(ConsoleMode mode) const {
    auto it = auth_devices.find(mode);
    return (it != auth_devices.end()) ? it->second : nullptr;
}

void AuthBroker::unregister_auth_device(ConsoleMode mode) {
    auto it = auth_devices.find(mode);
    if (it != auth_devices.end()) {
        auto device = std::move(it->second);
        auth_devices.erase(it);
    }
}

void AuthBroker::register_response_handler(ConsoleMode mode, AuthHandler handler) {
    unregister_response_handler(mode);
    response_handlers[mode] = std::move(handler);
}

void AuthBroker::unregister_response_handler(ConsoleMode mode) {
    auto it = response_handlers.find(mode);
    if (it != response_handlers.end()) {
        auto handler = std::move(it->second);
        response_handlers.erase(it);
    }
}

bool AuthBroker::forward_auth_response(ConsoleMode mode, XGIPProtocol* packet) {
    auto it = response_handlers.find(mode);
    if (it != response_handlers.end() && it->second) {
        it->second(packet);
        return true;
    }
    return false;
}

void AuthBroker::set_auth_completed(ConsoleMode mode, bool completed) {
    auth_completed[mode] = completed;
}

bool AuthBroker::is_auth_completed(ConsoleMode mode) const {
    auto it = auth_completed.find(mode);
    return it != auth_completed.end() && it->second;
}

bool AuthBroker::has_response_handler(ConsoleMode mode) const {
    return response_handlers.find(mode) != response_handlers.end();
}
