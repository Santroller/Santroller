#pragma once

#include <functional>
#include <map>
#include <memory>
#include "config/config.hpp"

// Forward declarations
typedef struct XGIPProtocol XGIPProtocol;
class UsbHostInterface;

/**
 * Auth Broker - Decouples USB device emulation from USB host for auth passthrough
 * 
 * USB Host devices register as auth providers for a specific console mode.
 * USB Device emulators call forward_auth when they receive auth packets from console.
 */
class AuthBroker {
public:
    using AuthHandler = std::function<void(XGIPProtocol*)>;
    
    /**
     * Register an auth handler for a specific console mode
     * Called by USB Host when it's ready to handle auth
     */
    void register_handler(ConsoleMode mode, AuthHandler handler);
    
    /**
     * Unregister auth handler for a console mode
     * Called by USB Host on disconnect
     */
    void unregister_handler(ConsoleMode mode);
    
    /**
     * Forward auth packet from console to registered host
     * Called by USB Device when it receives auth from console
     * Returns true if handler was found and called
     */
    bool forward_auth(ConsoleMode mode, XGIPProtocol* packet);
    
    /**
     * Check if an auth handler is registered for a mode
     */
    bool has_handler(ConsoleMode mode) const;
    
    /**
     * Register the actual USB host device for HID-style auth (PS4/PS5)
     * This allows emulation devices to get direct access for feature reports
     */
    void register_auth_device(ConsoleMode mode, std::shared_ptr<UsbHostInterface> device);
    
    /**
     * Get the registered auth device for HID-style auth
     * Returns nullptr if no device is registered
     */
    std::shared_ptr<UsbHostInterface> get_auth_device(ConsoleMode mode) const;
    
    /**
     * Unregister auth device for a console mode
     */
    void unregister_auth_device(ConsoleMode mode);
    
private:
    std::map<ConsoleMode, AuthHandler> handlers;
    std::map<ConsoleMode, std::shared_ptr<UsbHostInterface>> auth_devices;
};

// Global auth broker instance
extern AuthBroker auth_broker;
