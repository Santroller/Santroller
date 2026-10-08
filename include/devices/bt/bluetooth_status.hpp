#pragma once

// Whether anything is connected over bluetooth, in any role: as a controller (gamepad or
// Wiimote), or as a receiver with controllers connected to it
bool bluetooth_connected();
// Whether the bluetooth gamepad is connected to a host
bool bt_gamepad_connected();
// Whether the bluetooth gamepad is advertising, waiting for a host to connect
bool bt_gamepad_advertising();
