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

    /**
     * Register a response handler for a specific console mode
     * Called by the emulated (console-facing) USB device to receive real
     * auth responses from the host device, so they can be relayed back to
     * the console instead of the host faking an auth-complete locally.
     */
    void register_response_handler(ConsoleMode mode, AuthHandler handler, const void *owner = nullptr);

    /**
     * Unregister the response handler for a console mode
     */
    void unregister_response_handler(ConsoleMode mode);

    /**
     * Unregister the response handler for a console mode, but only if owner registered it.
     * Called by the emulated USB device on teardown: on a config reload the replacement
     * device registers before the old one is destroyed, so this must not remove its handler.
     */
    void unregister_response_handler(ConsoleMode mode, const void *owner);

    /**
     * Forward a real auth response packet from the host device to the
     * registered emulated (console-facing) device.
     * Returns true if a handler was found and called.
     */
    bool forward_auth_response(ConsoleMode mode, XGIPProtocol* packet);

    /**
     * Check if a response handler is registered for a mode
     */
    bool has_response_handler(ConsoleMode mode) const;

    /**
     * Track whether the console/PC has declared auth complete for a mode, so
     * controllers connected afterwards can be completed locally.
     */
    void set_auth_completed(ConsoleMode mode, bool completed);
    bool is_auth_completed(ConsoleMode mode) const;

    /**
     * Whether the console itself finished authenticating, unlike auth_completed this isn't set when
     * the host never started auth (eg a PC). Cleared when the console asks again.
     */
    void set_console_authenticated(ConsoleMode mode, bool authenticated);
    bool is_console_authenticated(ConsoleMode mode) const;

private:
    std::map<ConsoleMode, bool> auth_completed;
    std::map<ConsoleMode, bool> console_authenticated;
    std::map<ConsoleMode, AuthHandler> handlers;
    std::map<ConsoleMode, std::shared_ptr<UsbHostInterface>> auth_devices;
    std::map<ConsoleMode, AuthHandler> response_handlers;
    std::map<ConsoleMode, const void *> response_handler_owners;
};

// Global auth broker instance
extern AuthBroker auth_broker;
