#include "profiles/profile.hpp"
#include "mappings/base_mapping.hpp"
#include "leds/leds.hpp"

#include "input/shortcut.hpp"
#include <algorithm>
#include <cstdio>
#include "utils.h"

Profile::~Profile()
{
    queued_mappings.clear();
    action_mappings.clear();
    strum_mappings.clear();
    mappings.clear();
    triggers.clear();
    leds.clear();
    devices.clear();
}

void Profile::release_to_triggers()
{
    for (const auto &led : leds)
    {
        led->off();
    }
    queued_mappings.clear();
    queued_last_live.clear();
    action_mappings.clear();
    strum_mappings.clear();
    mappings.clear();
    leds.clear();
    triggers_only = true;
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

bool Profile::other_strum_live(int8_t strum_bit) const
{
    for (auto *mapping : strum_mappings)
    {
        if (mapping->strum_bit() != strum_bit && mapping->live_value())
        {
            return true;
        }
    }
    return false;
}

void Profile::update_actions()
{
    for (auto *mapping : action_mappings)
    {
        mapping->update_action();
    }
}

void Profile::reset_drum_state()
{
    drum_state.end_report(time_us_64());
    drum_state.reset();
}

void Profile::sample_input_queue()
{
    if (!input_queue.enabled)
    {
        return;
    }
    // Queued mappings are sampled before any shortcut builds a report, so let shortcuts mask
    // their queued members first or the press would already be in the queue
    for (auto &mapping : mappings)
    {
        if (mapping)
        {
            mapping->mask_shortcut_members(false, true);
        }
    }
    queued_last_live.resize(queued_mappings.size());
    uint16_t mask = 0;
    uint16_t retrigger = 0;
    for (size_t i = 0; i < queued_mappings.size(); i++)
    {
        auto *mapping = queued_mappings[i];
        mapping->sample(false, false);
        bool live = mapping->live_value();
        uint16_t bit = 1u << mapping->queue_bit();
        if (live)
        {
            mask |= bit;
            // Several inputs can share one output (eg both strums on a GuitarFreaks pick), so a new
            // press while another is still holding it needs a release first or it would be missed
            if (!queued_last_live[i] && (input_queue.last_pushed() & bit))
            {
                retrigger |= bit;
            }
        }
        queued_last_live[i] = live;
    }
    if (retrigger)
    {
        input_queue.push(input_queue.last_pushed() & ~retrigger);
    }
    input_queue.push(mask);
    input_queue.tick(micros());
}

int8_t InputQueue::bit_for_output(const proto_Output &output, SubType subtype)
{
    // bits 0-9 are frets (RB solo frets use 5-9), 10 / 11 are strum up / down
    switch (subtype)
    {
    case GuitarHeroGuitar:
        if (output.which_mapping == proto_Output_ghButton_tag &&
            output.mapping.ghButton >= GuitarHeroGuitar_Green && output.mapping.ghButton <= GuitarHeroGuitar_Orange)
        {
            return output.mapping.ghButton - GuitarHeroGuitar_Green;
        }
        break;
    case RockBandGuitar:
        if (output.which_mapping == proto_Output_rbButton_tag &&
            output.mapping.rbButton >= RockBandGuitar_Green && output.mapping.rbButton <= RockBandGuitar_SoloOrange)
        {
            return output.mapping.rbButton - RockBandGuitar_Green;
        }
        break;
    case GuitarFreaks:
        if (output.which_mapping == proto_Output_gfButton_tag && output.mapping.gfButton == GuitarFreaks_Strum)
        {
            return 10;
        }
        return -1;
    case LiveGuitar:
        if (output.which_mapping == proto_Output_ghlButton_tag)
        {
            if (output.mapping.ghlButton >= GuitarHeroLiveGuitar_White1 && output.mapping.ghlButton <= GuitarHeroLiveGuitar_Black3)
            {
                return output.mapping.ghlButton - GuitarHeroLiveGuitar_White1;
            }
            if (output.mapping.ghlButton == GuitarHeroLiveGuitar_StrumUp)
            {
                return 10;
            }
            if (output.mapping.ghlButton == GuitarHeroLiveGuitar_StrumDown)
            {
                return 11;
            }
        }
        break;
    default:
        return -1;
    }
    if (output.which_mapping == proto_Output_gamepadButton_tag)
    {
        if (output.mapping.gamepadButton == Gamepad_DpadUp)
        {
            return 10;
        }
        if (output.mapping.gamepadButton == Gamepad_DpadDown)
        {
            return 11;
        }
    }
    return -1;
}
