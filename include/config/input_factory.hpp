#pragma once
#include <memory>
#include <stdint.h>
#include "input.pb.h"
#include "profiles/profile.hpp"

class Input;

class InputFactory {
public:
    // Create an input from protobuf
    static std::unique_ptr<Input> create_input(
        const proto_Input& proto_input,
        std::shared_ptr<Profile> profile
    );
    
    template<typename T>
    static std::shared_ptr<T> get_device(std::shared_ptr<Profile> profile, uint32_t device_id) {
        constexpr DeviceSlotKind slot_kind = T::SLOT_KIND;

        if constexpr (slot_kind != DeviceSlotKind::None) {
            if (auto claimed = profile->get_claimed_device(slot_kind, static_cast<uint16_t>(device_id))) {
                return device_cast<T>(claimed);
            }
            // Legacy profiles store the root device id rather than a slot id, so fall back to
            // whatever of this kind the profile claimed, as firmware before slots did
            if (!profile->per_kind_slot_ids) {
                for (const auto &claimed : profile->claimed_devices) {
                    if (claimed.first.first == slot_kind) {
                        if (auto device = device_cast<T>(claimed.second)) {
                            return device;
                        }
                    }
                }
            }
        }
        auto s_it = profile->devices.find(device_id);
        if (s_it != profile->devices.end()) {
            return device_cast<T>(s_it->second);
        }
        return nullptr;
    }

    template<typename InputType, typename DeviceType, typename ConfigType>
    static std::unique_ptr<Input> create_midi_input(
        std::shared_ptr<Profile> profile,
        DeviceSlotKind source_kind,
        const ConfigType &config)
    {
        auto device = profile->get_midi_source_device(
            static_cast<uint16_t>(config.deviceid),
            source_kind);
        return std::make_unique<InputType>(
            config,
            device_cast<DeviceType>(device),
            profile.get());
    }
};
