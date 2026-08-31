#include "config/input_factory.hpp"
#include "profiles/profile.hpp"
#include "input/wii.hpp"
#include "input/matrix.hpp"
#include "input/vtechexpander.hpp"
#include "input/crkd.hpp"
#include "input/crkd_drum.hpp"
#include "input/gpio.hpp"
#include "input/ads1115.hpp"
#include "input/multiplexer.hpp"
#include "input/accelerometer.hpp"
#include "input/gh5.hpp"
#include "input/fixed.hpp"
#include "input/ps2.hpp"
#include "input/mpr121.hpp"
#include "input/midi.hpp"
#include "input/protarNeck.hpp"
#include "input/usb.hpp"

template<typename InputType, typename DeviceType, typename ConfigType>
static std::unique_ptr<Input> create_device_input(std::shared_ptr<Profile> profile, uint32_t device_id, const ConfigType &config)
{
    auto device = InputFactory::get_device<DeviceType>(profile, device_id);
    return device ? std::make_unique<InputType>(config, device) : nullptr;
}

template<typename InputType, typename ConfigType>
static std::unique_ptr<Input> create_simple_input(const ConfigType &config)
{
    return std::make_unique<InputType>(config);
}

bool InputFactory::has_device(std::shared_ptr<Profile> profile, uint32_t device_id)
{
    return profile->devices.find(device_id) != profile->devices.end();
}

std::unique_ptr<Input> InputFactory::create_input(
    const proto_Input& proto_input,
    std::shared_ptr<Profile> profile)
{
    switch (proto_input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        return create_device_input<WiiAxisInput, WiiDevice>(profile, proto_input.input.wiiAxis.deviceid, proto_input.input.wiiAxis);
            
    case proto_Input_matrix_tag:
        return create_device_input<MatrixInput, MatrixDevice>(profile, proto_input.input.matrix.deviceid, proto_input.input.matrix);
            
    case proto_Input_wiiButton_tag:
        return create_device_input<WiiButtonInput, WiiDevice>(profile, proto_input.input.wiiButton.deviceid, proto_input.input.wiiButton);
            
    case proto_Input_vtechExpander_tag:
        return create_device_input<VTechExpanderInput, VTechGuitarIOExpanderDevice>(profile, proto_input.input.vtechExpander.deviceid, proto_input.input.vtechExpander);
            
    case proto_Input_crkd_tag:
        return create_device_input<CrkdButtonInput, CrkdDevice>(profile, proto_input.input.crkd.deviceid, proto_input.input.crkd);
            
    case proto_Input_crkdDrum_tag:
        return create_device_input<CrkdDrumInput, CrkdDrumDevice>(profile, proto_input.input.crkdDrum.deviceid, proto_input.input.crkdDrum);
            
    case proto_Input_gpio_tag:
        return create_simple_input<GPIOInput>(proto_input.input.gpio);
        
    case proto_Input_ads1115_tag:
        return create_device_input<ADS1115Input, ADS1115Device>(profile, proto_input.input.ads1115.deviceid, proto_input.input.ads1115);
            
    case proto_Input_multiplexer_tag:
        return create_device_input<MultiplexerInput, MultiplexerDevice>(profile, proto_input.input.multiplexer.deviceid, proto_input.input.multiplexer);
            
    case proto_Input_accelerometer_tag:
        return create_device_input<AccelerometerInput, AccelerometerDevice>(profile, proto_input.input.accelerometer.deviceid, proto_input.input.accelerometer);
            
    case proto_Input_gh5Neck_tag:
        return create_device_input<Gh5ButtonInput, GH5NeckDevice>(profile, proto_input.input.gh5Neck.deviceid, proto_input.input.gh5Neck);
            
    case proto_Input_fixed_tag:
        return create_simple_input<FixedInput>(proto_input.input.fixed);
        
    case proto_Input_ps2Axis_tag:
        return create_device_input<PS2AxisInput, PS2Device>(profile, proto_input.input.ps2Axis.deviceid, proto_input.input.ps2Axis);
            
    case proto_Input_ps2Button_tag:
        return create_device_input<PS2ButtonInput, PS2Device>(profile, proto_input.input.ps2Button.deviceid, proto_input.input.ps2Button);
            
    case proto_Input_mpr121_tag:
        return create_device_input<MPR121Input, MPR121Device>(profile, proto_input.input.mpr121.deviceid, proto_input.input.mpr121);
            
    case proto_Input_midi_tag:
    {
        if (!has_device(profile, proto_input.input.midi.deviceid)) return nullptr;
        
        switch (proto_input.input.midi.which_input)
        {
        case proto_MidiInput_midiNote_tag:
            return create_device_input<MidiNoteInput, MidiDevice>(profile, proto_input.input.midi.deviceid, proto_input.input.midi.input.midiNote);
        case proto_MidiInput_midiControlChange_tag:
            return create_device_input<MidiControlChangeInput, MidiDevice>(profile, proto_input.input.midi.deviceid, proto_input.input.midi.input.midiControlChange);
        case proto_MidiInput_midiPitchBend_tag:
            return create_device_input<MidiPitchBendInput, MidiDevice>(profile, proto_input.input.midi.deviceid, proto_input.input.midi.input.midiPitchBend);
        case proto_MidiInput_midiProGuitarButton_tag:
            return create_device_input<MidiProGuitarButtonInput, ProGuitarMidiDevice>(profile, proto_input.input.midi.deviceid, proto_input.input.midi.input.midiProGuitarButton);
        case proto_MidiInput_midiProGuitarAxis_tag:
            return create_device_input<MidiProGuitarAxisInput, ProGuitarMidiDevice>(profile, proto_input.input.midi.deviceid, proto_input.input.midi.input.midiProGuitarAxis);
        }
        return nullptr;
    }
    
    case proto_Input_protarNeckButton_tag:
        return create_device_input<ProtarNeckButtonInput, ProtarNeckDevice>(profile, proto_input.input.protarNeckButton.deviceid, proto_input.input.protarNeckButton);
            
    case proto_Input_protarNeckAxis_tag:
        return create_device_input<ProtarNeckAxisInput, ProtarNeckDevice>(profile, proto_input.input.protarNeckAxis.deviceid, proto_input.input.protarNeckAxis);
            
    case proto_Input_mouseAxis_tag:
        return create_device_input<MouseAxisInput, UsbHostInterface>(profile, proto_input.input.mouseAxis.deviceid, proto_input.input.mouseAxis);
            
    case proto_Input_mouseButton_tag:
        return create_device_input<MouseButtonInput, UsbHostInterface>(profile, proto_input.input.mouseButton.deviceid, proto_input.input.mouseButton);
            
    case proto_Input_key_tag:
        return create_device_input<KeyboardKeyInput, UsbHostInterface>(profile, proto_input.input.key.deviceid, proto_input.input.key);
            
    case proto_Input_usbButton_tag:
        return create_device_input<USBButtonInput, UsbHostInterface>(profile, proto_input.input.usbButton.deviceid, proto_input.input.usbButton);
            
    case proto_Input_usbAxis_tag:
        return create_device_input<USBAxisInput, UsbHostInterface>(profile, proto_input.input.usbAxis.deviceid, proto_input.input.usbAxis);
            
    // Special inputs (held, cycle, toggle, shortcut) are handled by callbacks
    case proto_Input_held_tag:
    case proto_Input_cycle_tag:
    case proto_Input_toggle_tag:
    case proto_Input_shortcut_tag:
    case 0:
        // These are handled specially in config.cpp with callbacks
        return nullptr;
        
    default:
        return nullptr;
    }
}
