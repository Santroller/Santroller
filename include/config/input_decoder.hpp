#pragma once

#include "input.pb.h"
#include "input/input.hpp"
#include <memory>
#include <pb_decode.h>

class Profile;

// Each Input field needs its own result, including siblings inside the same
// modifier.
struct InputDecoder {
  std::shared_ptr<Profile> profile;
  std::unique_ptr<Input> special;
  bool has_legacy_cycle_trigger = false;
  proto_AnalogToDigitalTriggerType legacy_cycle_trigger =
      AnalogToDigitalTriggerType_JoyHigh;
  bool has_legacy_cycle_trigger_value = false;
  int32_t legacy_cycle_trigger_value = 0;
  bool has_legacy_cycle_max_trigger_value = false;
  int32_t legacy_cycle_max_trigger_value = 0;

  std::unique_ptr<Input> take(const proto_Input &input);
  static bool prepare(pb_istream_t *stream, const pb_field_t *field,
                      void **arg);
  static bool decode_special(pb_istream_t *stream, const pb_field_t *field,
                             void **arg);
};
