#pragma once

// While no profile matches (standby lighting aside) and the tool isn't open, BOOTSEL does what the tool's pair button does
// (the scan command): bluetooth discovery, 360 RF sync.
void bootsel_pairing_update();
