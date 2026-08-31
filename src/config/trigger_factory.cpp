#include "config/trigger_factory.hpp"
#include "profiles/profile.hpp"
#include "triggers/activation_trigger.hpp"
#include "triggers/input_trigger.hpp"
#include "triggers/device_type_triggers.hpp"
#include "triggers/mode_triggers.hpp"

std::unique_ptr<ActivationTrigger> TriggerFactory::create_trigger(
    const proto_ProfileAssignmentInfo& proto_assignment,
    std::shared_ptr<Profile> profile,
    std::unique_ptr<Input> input,
    uint32_t trigger_id,
    uint32_t list_id)
{
    switch (proto_assignment.which_assignment)
    {
    case proto_ProfileAssignmentInfo_input_tag:
        if (!input) return nullptr;
        return std::make_unique<InputActivationTrigger>(
            false,
            proto_assignment.assignment.input,
            std::move(input),
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_inputAnyTime_tag:
        if (!input) return nullptr;
        return std::make_unique<InputActivationTrigger>(
            true,
            proto_assignment.assignment.inputAnyTime,
            std::move(input),
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_consoleType_tag:
        return std::make_unique<UsbModeActivationTrigger>(
            proto_assignment.assignment.consoleType,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_wiiExt_tag:
        return std::make_unique<WiiExtTypeActivationTrigger>(
            proto_assignment.assignment.wiiExt,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_ps2Cnt_tag:
        return std::make_unique<PS2ControllerTypeActivationTrigger>(
            proto_assignment.assignment.ps2Cnt,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_usbType_tag:
        return std::make_unique<UsbTypeActivationTrigger>(
            proto_assignment.assignment.usbType,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_usbDevice_tag:
        return std::make_unique<SpecificUsbDeviceActivationTrigger>(
            proto_assignment.assignment.usbDevice,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_midiChannel_tag:
        return std::make_unique<MidiChannelActivationTrigger>(
            proto_assignment.assignment.midiChannel,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_ps2Emulation_tag:
        return std::make_unique<PS2ControllerEmulationActivationTrigger>(
            proto_assignment.assignment.ps2Emulation,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_wiiEmulation_tag:
        return std::make_unique<WiiExtensionEmulationActivationTrigger>(
            proto_assignment.assignment.wiiEmulation,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_bluetooth_tag:
        return std::make_unique<BluetoothModeActivationTrigger>(
            proto_assignment.assignment.bluetooth,
            profile,
            trigger_id,
            list_id
        );
        
    case proto_ProfileAssignmentInfo_copilotProfile_tag:
        // TODO: how do we handle this
        return nullptr;
        
    default:
        return nullptr;
    }
}
