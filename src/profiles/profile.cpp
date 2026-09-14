#include "profiles/profile.hpp"
#include "mappings/base_mapping.hpp"
#include "leds/leds.hpp"

#include "input/shortcut.hpp"
#include <algorithm>
#include <cstdio>

Profile::~Profile()
{
    mappings.clear();
    triggers.clear();
    leds.clear();
    devices.clear();
}

void Profile::resolve_shortcuts()
{
    for (auto &mapping : mappings)
    {
        if (mapping)
        {
            mapping->clear_masked_mappings();
        }
    }

    // Move all shortcut mappings to the beginning of mappings so they evaluate
    // before the base mappings they mask.
    std::stable_partition(mappings.begin(), mappings.end(), [](const std::unique_ptr<Mapping> &m) {
        return m && m->get_input() && m->get_input()->as_shortcut() != nullptr;
    });

    for (auto &shortcut_mapping : mappings)
    {
        if (!shortcut_mapping || !shortcut_mapping->get_input())
        {
            continue;
        }
        auto *shortcut = shortcut_mapping->get_input()->as_shortcut();
        if (!shortcut)
        {
            continue;
        }

        for (const auto &child_input : shortcut->get_inputs())
        {
            if (!child_input)
            {
                continue;
            }
            uint64_t hid = child_input->hardware_id();
            if (hid == 0)
            {
                continue;
            }

            for (auto &other_mapping : mappings)
            {
                if (other_mapping.get() == shortcut_mapping.get())
                {
                    continue;
                }
                if (!other_mapping || !other_mapping->get_input())
                {
                    continue;
                }
                if (other_mapping->get_input()->as_shortcut() != nullptr)
                {
                    continue;
                }

                if (other_mapping->get_input()->hardware_id() == hid)
                {
                    shortcut_mapping->add_masked_mapping(other_mapping.get());
                    printf("Shortcut mapping (id=%u) masks base mapping (id=%u)\n",
                           shortcut_mapping->id(), other_mapping->id());
                }
            }
        }
    }
}
