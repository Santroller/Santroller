#pragma once
// Fake managers/inactivity_manager.hpp
#include "config_fakes.hpp"

class InactivityManager
{
public:
    static InactivityManager &instance()
    {
        static InactivityManager manager;
        return manager;
    }
    void configure(const proto_InactivityConfig *config)
    {
        fake_loader::state.inactivity_configured = true;
        fake_loader::state.inactivity_had_config = config != nullptr;
        if (config)
        {
            fake_loader::state.inactivity = *config;
        }
    }
};
