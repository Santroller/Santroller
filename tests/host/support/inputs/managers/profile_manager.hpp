#pragma once
// Fake managers/profile_manager.hpp: the device triggers only ask it for the instances a profile
// is running on, so they can push that instance's LEDs / rumble onto a newly claimed device
#include <map>
#include <optional>
#include <utility>
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
    void set_xbox_one_auth_pending(uint32_t profile_id, uint32_t list_id) { xbox_one_auth_pending = {profile_id, list_id}; }
    bool is_xbox_one_auth_holder(uint32_t profile_id, uint32_t list_id) const
    {
        return xbox_one_auth_holder == std::make_pair(profile_id, list_id);
    }
    // Set by tests
    std::pair<uint32_t, uint32_t> xbox_one_auth_pending;
    std::optional<std::pair<uint32_t, uint32_t>> xbox_one_auth_holder;
    std::map<uint32_t, std::vector<std::shared_ptr<Instance>>> instances;
    std::vector<std::shared_ptr<Profile>> profiles;
};
