#pragma once
#include <stdint.h>
#include "instance.hpp"

// Device side of the Santroller output commands (see protocols/santroller_output.hpp).
// Accepts the command with or without the leading report id. Returns false if the data
// isn't a Santroller command, so callers can fall back to their older report layouts.
bool santroller_handle_output_command(Instance &instance, const uint8_t *data, uint16_t len);
