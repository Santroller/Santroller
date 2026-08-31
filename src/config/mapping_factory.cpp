#include "config/mapping_factory.hpp"
#include "managers/config_manager.hpp"
#include "profiles/profile.hpp"
#include "mappings/mapping.hpp"

static auto& config_mgr = ConfigManager::instance();

std::unique_ptr<Mapping> MappingFactory::create_mapping(
    const proto_Mapping& proto_mapping,
    std::shared_ptr<Profile> profile,
    std::unique_ptr<Input> input,
    uint32_t mapping_id)
{
    if (!input) {
        return nullptr;
    }
    
    switch (proto_mapping.mapping.which_mapping)
    {
    case proto_Output_gamepadAxis_tag:
        if (profile->subtype == Gamepad && config_mgr.get_mode() == ModePs3) {
            return std::make_unique<PS3GamepadAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        // XB1 guitars and drums don't use the same report format as gamepads
        if (profile->subtype == GuitarHeroGuitar || profile->subtype == RockBandGuitar) {
            return std::make_unique<RockBandGuitarGamepadAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        if (profile->subtype == RockBandDrums) {
            return std::make_unique<RockBandDrumsGamepadAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        return std::make_unique<GamepadAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghAxis_tag:
        return std::make_unique<GuitarHeroGuitarAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghButton_tag:
        return std::make_unique<GuitarHeroGuitarButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_rbAxis_tag:
        if (profile->subtype == PowerGigGuitar) {
            return std::make_unique<PowerGigGuitarAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        return std::make_unique<RockBandGuitarAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_rbButton_tag:
        if (profile->subtype == PowerGigGuitar) {
            return std::make_unique<PowerGigGuitarButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        return std::make_unique<RockBandGuitarButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_gamepadButton_tag:
        // PS2 GH guitars don't use the same report format as gamepads
        if (profile->subtype == GuitarHeroGuitar) {
            return std::make_unique<GuitarHeroGuitarGamepadButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        // PS3 gamepads use a totally different report format
        if (profile->subtype == Gamepad && config_mgr.get_mode() == ModePs3) {
            return std::make_unique<PS3GamepadButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        return std::make_unique<GamepadButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghDrumAxis_tag:
        return std::make_unique<GuitarHeroDrumsAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_rbDrumButton_tag:
        if (profile->subtype == PowerGigDrum) {
            return std::make_unique<PowerGigDrumsButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        return std::make_unique<RockBandDrumsButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_rbDrumAxis_tag:
        if (profile->subtype == PowerGigDrum) {
            return std::make_unique<PowerGigDrumsAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        }
        return std::make_unique<RockBandDrumsAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghlButton_tag:
        return std::make_unique<LiveGuitarButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghlAxis_tag:
        return std::make_unique<LiveGuitarAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_proButton_tag:
        return std::make_unique<ProGuitarButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_proAxis_tag:
        return std::make_unique<ProGuitarAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_djhButton_tag:
        return std::make_unique<DJHTurntableButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_djhAxis_tag:
        return std::make_unique<DJHTurntableAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_djMaxButton_tag:
        return std::make_unique<DJMaxTurntableButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_djMaxAxis_tag:
        return std::make_unique<DJMaxTurntableAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_divaAxis_tag:
        return std::make_unique<ProjectDivaAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_gfButton_tag:
        return std::make_unique<GuitarFreaksButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghaAxis_tag:
        return std::make_unique<GuitarHeroArcadeAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_ghaButton_tag:
        return std::make_unique<GuitarHeroArcadeButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_mouseAxis_tag:
        return std::make_unique<MouseAxisMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_mouseButton_tag:
        return std::make_unique<MouseButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    case proto_Output_keycode_tag:
        return std::make_unique<KeyboardButtonMapping>(proto_mapping, std::move(input), mapping_id, profile);
        
    default:
        return nullptr;
    }
}
