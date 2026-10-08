#pragma once

// Whether any Xbox 360 wireless receiver slot has a controller linked to it. Bluetooth uses this to keep
// background scanning off while one is, as our radio shares 2.4GHz with the 360's link.
// Kept free of TinyUSB types so the bluetooth code can include it.
bool xinput_wireless_controller_linked();
