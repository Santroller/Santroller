#pragma once
// Fake managers/profile_manager.hpp: the device triggers only ask it for the instances a profile
// is running on, so they can push that instance's LEDs / rumble onto a newly claimed device
#include <map>
#include <memory>
#include <vector>
#include "instance.hpp"

class ProfileManager
{
public:
    static ProfileManager &instance()
    {
        static ProfileManager manager;
        return manager;
    }
    std::vector<std::shared_ptr<Instance>> get_instances_for_profile(uint32_t profile_id) const
    {
        auto it = instances.find(profile_id);
        return it == instances.end() ? std::vector<std::shared_ptr<Instance>>{} : it->second;
    }
    template <typename Func>
    void for_each_profile(Func func) const
    {
        for (const auto &profile : profiles)
        {
            func(profile->profile_id, profile);
        }
    }
    // Set by tests
    std::map<uint32_t, std::vector<std::shared_ptr<Instance>>> instances;
    std::vector<std::shared_ptr<Profile>> profiles;
};
