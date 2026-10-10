#pragma once

// Whether anything is connected over bluetooth, in any role: as a controller (gamepad or
// Wiimote), or as a receiver with controllers connected to it
bool bluetooth_connected();
// Whether a host (PC, phone, or a Wii via Wiimote emulation) is connected to this controller,
// including a configurator connected over BLE while no profile outputs over bluetooth
bool bluetooth_host_connected();
// Whether the bluetooth gamepad is connected to a host
bool bt_gamepad_connected();
// Whether a host is connected to the BLE peripheral, for gamepad output or for config
bool bt_peripheral_connected();
// Whether the bluetooth gamepad is advertising, waiting for a host to connect
bool bt_gamepad_advertising();
