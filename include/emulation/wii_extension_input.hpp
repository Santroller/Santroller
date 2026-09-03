#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include "profiles/profile.hpp"
#include "device.pb.h"

uint8_t wii_extension_format_for_subtype(SubType subtype, uint8_t detected_format);
uint8_t wii_extension_report_size(SubType subtype, uint8_t format);
void initialize_wii_extension_report(SubType subtype, uint8_t format,
                                      uint8_t *report, uint8_t *size,
                                      uint8_t *buttons_low_offset,
                                      uint8_t *buttons_high_offset);
void finalize_wii_extension_report(uint8_t *report,
                                   uint8_t buttons_low_offset,
                                   uint8_t buttons_high_offset);

void update_wii_extension_input(const std::vector<std::shared_ptr<Profile>> &profiles,
                                bool full_poll, bool send_events,
                                uint8_t format, uint8_t *report);
