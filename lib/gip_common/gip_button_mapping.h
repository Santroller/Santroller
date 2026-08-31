#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "device.pb.h"
#include "input.pb.h"

#ifdef __cplusplus
#include "../../include/protocols/xbox_one.hpp"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shared tick_digital implementation for Xbox One controllers
 * Works with both wired and wireless implementations
 * 
 * @param input_data Pointer to controller input data (XboxOneGamepad_Data_t, etc.)
 * @param subtype Device subtype (SubType_Gamepad, SubType_RockBandGuitar, etc.)
 * @param type Output mapping type
 * @return true if button is pressed, false otherwise
 */
bool gip_tick_digital(const void *input_data, uint8_t subtype, proto_Output *type);

/**
 * Shared tick_analog implementation for Xbox One controllers
 * Works with both wired and wireless implementations
 * 
 * @param input_data Pointer to controller input data (XboxOneGamepad_Data_t, etc.)
 * @param subtype Device subtype (SubType_Gamepad, SubType_RockBandGuitar, etc.)
 * @param type Output mapping type
 * @return Analog value (0-65535)
 */
uint16_t gip_tick_analog(const void *input_data, uint8_t subtype, proto_Output *type);

#ifdef __cplusplus
}
#endif
