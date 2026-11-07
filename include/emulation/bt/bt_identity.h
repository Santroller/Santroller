#pragma once
#include <stdint.h>

// Each kind of bluetooth peripheral we can be (a gamepad, a dance pad, a keyboard / mouse, or just the config
// service) advertises from its own address, so hosts see it as its own device.
//
// Hosts cache a bonded device's GATT layout and HID report map, and don't reliably notice either changing:
// BlueZ's HoG starts from the cached report map and handles before it checks the database hash, and keeps
// using them after it rediscovers, and Windows binds the HID device once. With an address per kind, a host
// never sees one change under the same address, so nothing needs to be unpaired when the mode changes.
//
// The gamepad keeps the controller's public address, so bonds from before this existed still work. The others
// use a random static address derived from it, so they stay the same across reboots.
//
// A host that pairs with two of them is bonded twice, with different keys. Each host bond remembers which
// identity it was made with, and only the current identity's bonds are visible to btstack, so the right keys are
// used and pairing one doesn't replace the other. Bonds with controllers we connect to as a central are visible
// whichever identity is in use.
enum BtIdentity : uint8_t
{
    BtIdentityGamepad = 0,
    BtIdentityDancepad = 1,
    BtIdentityKeyboard = 2,
    BtIdentityConfigOnly = 3,
};

// Call once btstack's device db is configured, with the stack lock held
void bt_identity_init();
// Switches the identity advertised from, and which host bonds btstack sees. Call with the stack lock held,
// while no host is connected to the peripheral (the bonds it is using would disappear)
void bt_identity_set(BtIdentity identity);
// Shows every bond, whatever identity it was made with, until it is turned back on. For forgetting all hosts.
// Call with the stack lock held.
void bt_identity_filter_bonds(bool filter);
