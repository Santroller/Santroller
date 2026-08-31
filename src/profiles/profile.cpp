#include "profiles/profile.hpp"
#include "mappings/base_mapping.hpp"
#include "leds/leds.hpp"

// Destructor must be defined in .cpp where Mapping and LedMapping are complete types
// This is required for unique_ptr<Mapping> and unique_ptr<LedMapping> to work
Profile::~Profile() = default;
