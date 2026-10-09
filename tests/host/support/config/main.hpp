#pragma once
// Fake main.hpp
#include "config_fakes.hpp"

inline void reinitialize_device_stack() { fake_loader::state.reinitialize_device_stack_calls++; }
