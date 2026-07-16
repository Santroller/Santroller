#include "config.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "input/fixed.hpp"
#include "input/wii.hpp"
#include "input/mpr121.hpp"
#include "input/ps2.hpp"
#include "input/midi.hpp"
#include "input/protarNeck.hpp"
#include "input/held.hpp"
#include "input/cycle.hpp"
#include "input/toggle.hpp"
#include "input/usb.hpp"
#include "input/crkd.hpp"
#include "input/crkd_drum.hpp"
#include "input/matrix.hpp"
#include "input/shortcut.hpp"
#include "input/ads1115.hpp"
#include "input/accelerometer.hpp"
#include "input/multiplexer.hpp"
#include "input/vtechexpander.hpp"
#include "input/gh5.hpp"
#include "devices/base.hpp"
#include "devices/accelerometer.hpp"
#include "devices/multiplexer.hpp"
#include "devices/wii.hpp"
#include "devices/bhdrum.hpp"
#include "devices/wtdrum.hpp"
#include "devices/crazyneck.hpp"
#include "devices/debug.hpp"
#include "devices/djh.hpp"
#include "devices/crkd.hpp"
#include "devices/crkd_drum.hpp"
#include "devices/cycle.hpp"
#include "devices/ads1115.hpp"
#include "devices/protar_neck.hpp"
#include "devices/gh5neck.hpp"
#include "devices/usb.hpp"
#include "devices/matrix.hpp"
#include "devices/ps2.hpp"
#include "devices/max1704x.hpp"
#include "devices/vtechexpander.hpp"
#include "devices/mpr121.hpp"
#include "devices/ws2812.hpp"
#include "devices/apa102.hpp"
#include "devices/midiserial.hpp"
#include "devices/stp16cpc.hpp"
#include "devices/bluetooth.hpp"
#include "devices/dmx.hpp"
#include "mappings/mapping.hpp"
#include "leds/leds.hpp"
#include "tusb.h"
#include "usb/device/xinput_device.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/xone_device.h"
#include "usb/device/hid_device.h"
#include "usb/device/ps3_device.h"
#include "usb/device/ps4_device.h"
#include "usb/device/ps5_device.h"
#include "usb/device/gh_arcade_device.h"
#include "usb/device/switch_device.h"
#include "bt/device/bt_gamepad.h"
#include "usb/host/host.hpp"
#include "usb/usb_descriptors.h"
#include "hardware/watchdog.h"
#include "main.hpp"
#include "host/usbh.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "quadrature_encoder.h"
#include <vector>
#include <set>
#include <memory>
#include <algorithm>
std::vector<std::shared_ptr<Instance>> instances;
std::vector<std::shared_ptr<Instance>> active_instances;
std::unordered_map<uint32_t, std::shared_ptr<Profile>> all_profiles;
std::set<uint32_t> active_profiles;
std::vector<std::shared_ptr<Device>> active_devices;
std::unordered_map<uint32_t, std::shared_ptr<Device>> root_devices;
// devices that have not yet been assigned to a profile
std::vector<std::shared_ptr<Device>> assignable_devices;
std::vector<std::shared_ptr<UsbHostInterface>> assignable_usb_devices;
std::vector<std::shared_ptr<UsbHostInterface>> enumerating_usb_devices;
std::map<ConsoleMode, std::shared_ptr<UsbHostInterface>> auth_devices;
std::shared_ptr<UsbDevice> usb_instances[32];
std::shared_ptr<UsbDevice> usb_instances_by_epin[16];
std::shared_ptr<UsbDevice> usb_instances_by_epout[16];
std::map<int32_t, int32_t> cycle_input_states;
std::map<int32_t, bool> toggle_input_states;
std::vector<uint32_t> last_cycle_states;
ConsoleMode mode = ModeHid;
ConsoleMode newMode = mode;
std::shared_ptr<Profile> working_profile;
int seenMasks = 0;
bool fullReload = false;
bool working = false;
bool loadedAny = false;
bool load_cycle_state(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint32_t state;
    auto ret = pb_decode_varint32(stream, &state);
    last_cycle_states.push_back(state);
    return ret;
}
bool load_device_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    if (field->tag == proto_Device_cycle_tag)
    {
        proto_CycleDevice *msg = (proto_CycleDevice *)field->pData;
        msg->values.funcs.decode = load_cycle_state;
    }
    return true;
}
bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    printf("load_device\r\n");
    proto_Device device proto_Device_init_zero;
    device.cb_device.funcs.decode = load_device_dev;
    pb_decode(stream, proto_Device_fields, &device);
    auto dev_id = device.deviceid;
    // If we are loading a new config, we grab the previous device so we can make sure its state is restored
    auto prevDevice = std::shared_ptr<Device>();
    printf("root_devices: %d\r\n", root_devices.size());
    if (auto it = root_devices.find(dev_id); it != root_devices.end())
    {
        prevDevice = it->second;
        // signal devices that they are being torn down, but in a way where if they are being replaced, they aren't fully torn down
        // This is so that things like PS2 controllers, Wii extensions and USB host and bluetooth aren't restarted during a config change
        prevDevice->end(false);
    }
    printf("found device! %p\r\n", prevDevice);
    printf("device id: %d, type: %d\r\n", dev_id, device.which_device);
    switch (device.which_device)
    {
    case proto_Device_accelerometer_tag:
        active_devices.emplace_back(new AccelerometerDevice(device.device.accelerometer, dev_id));
        break;
    case proto_Device_crkdNeck_tag:
        active_devices.emplace_back(new CrkdDevice(device.device.crkdNeck, dev_id));
        break;
    case proto_Device_crkdDrum_tag:
        return true;
        // active_devices.emplace_back(new CrkdDrumDevice(device.device.crkdDrum, dev_id));
        break;
    case proto_Device_wii_tag:
        // we pass in the previous device here so we can make sure the state is kept between reloads
        // that way, an extension stays connected between reloads
        active_devices.emplace_back(new WiiDevice(std::static_pointer_cast<WiiDevice>(prevDevice), device.device.wii, dev_id));
        break;
    case proto_Device_psx_tag:
        // we pass in the previous device here so we can make sure the state is kept between reloads
        // that way, a controller stays connected between reloads
        active_devices.emplace_back(new PS2Device(std::static_pointer_cast<PS2Device>(prevDevice), device.device.psx, dev_id));
        break;
    case proto_Device_protarNeck_tag:
        active_devices.emplace_back(new ProtarNeckDevice(device.device.protarNeck, dev_id));
        break;
    case proto_Device_bhDrum_tag:
        active_devices.emplace_back(new BandHeroDrumDevice(device.device.bhDrum, dev_id));
        break;
    case proto_Device_worldTourDrum_tag:
        active_devices.emplace_back(new WorldTourDrumDevice(device.device.worldTourDrum, dev_id));
        break;
    case proto_Device_crazyGuitarNeck_tag:
        active_devices.emplace_back(new CrazyGuitarNeckDevice(device.device.crazyGuitarNeck, dev_id));
        break;
    case proto_Device_djhTurntable_tag:
        active_devices.emplace_back(new DjHeroTurntableDevice(device.device.djhTurntable, dev_id));
        break;
    case proto_Device_gh5Neck_tag:
        active_devices.emplace_back(new GH5NeckDevice(device.device.gh5Neck, dev_id));
        break;
    case proto_Device_max1704x_tag:
        active_devices.emplace_back(new Max1704XDevice(device.device.max1704x, dev_id));
        break;
    case proto_Device_mpr121_tag:
        active_devices.emplace_back(new MPR121Device(device.device.mpr121, dev_id));
        break;
    case proto_Device_usbHost_tag:
        active_devices.emplace_back(new USBHostHardwareDevice(device.device.usbHost, dev_id));
        break;
    case proto_Device_ads1115_tag:
        active_devices.emplace_back(new ADS1115Device(device.device.ads1115, dev_id));
        break;
    case proto_Device_debug_tag:
        active_devices.emplace_back(new DebugDevice(device.device.debug, dev_id));
        break;
    case proto_Device_midiSerial_tag:
        active_devices.emplace_back(new MidiSerialDevice(device.device.midiSerial, dev_id));
        break;
    case proto_Device_ws2812_tag:
        return true;
        active_devices.emplace_back(new WS2812Device(device.device.ws2812, dev_id));
        break;
    case proto_Device_stp16cpc_tag:
        active_devices.emplace_back(new STP16CPCDevice(device.device.stp16cpc, dev_id));
        break;
    case proto_Device_apa102_tag:
        active_devices.emplace_back(new APA102Device(device.device.apa102, dev_id));
        break;
    case proto_Device_multiplexer_tag:
        active_devices.emplace_back(new MultiplexerDevice(device.device.multiplexer, dev_id));
        break;
    case proto_Device_bt_tag:
        active_devices.emplace_back(new BluetoothDevice(device.device.bt, dev_id));
        break;
    case proto_Device_vtechExpander_tag:
        active_devices.emplace_back(new VTechGuitarIOExpanderDevice(device.device.vtechExpander, dev_id));
        break;
    case proto_Device_matrix_tag:
        active_devices.emplace_back(new MatrixDevice(device.device.matrix, dev_id));
        break;
    case proto_Device_cycle_tag:
        active_devices.emplace_back(new CycleDevice(device.device.cycle, dev_id, cycle_input_states[dev_id], last_cycle_states));
        last_cycle_states.clear();
        break;
    case proto_Device_toggle_tag:
        active_devices.emplace_back(new ToggleDevice(device.device.toggle, dev_id, toggle_input_states[dev_id]));
        break;
    case proto_Device_dmx_tag:
        active_devices.emplace_back(new DMXDevice(device.device.dmx, dev_id));
        break;
    }
    root_devices[dev_id] = active_devices.back();
    active_devices.back()->still_connected = true;
    active_devices.back()->begin();
    active_devices.back()->rescan(true);
    return true;
}
ShortcutInput *last_shortcut = nullptr;
Input *last_special = nullptr;

std::unique_ptr<Input> make_input(proto_Input input, std::shared_ptr<Profile> profile, pb_istream_t *stream)
{
    printf("make input: %d %p\r\n", input.which_input, profile.get());
    switch (input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        if (profile->devices.find(input.input.wiiAxis.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new WiiAxisInput(input.input.wiiAxis, std::static_pointer_cast<WiiDevice>(profile->devices[input.input.wiiAxis.deviceid])));
    case proto_Input_matrix_tag:
        if (profile->devices.find(input.input.matrix.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MatrixInput(input.input.matrix, std::static_pointer_cast<MatrixDevice>(profile->devices[input.input.matrix.deviceid])));
    case proto_Input_wiiButton_tag:
        if (profile->devices.find(input.input.wiiButton.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new WiiButtonInput(input.input.wiiButton, std::static_pointer_cast<WiiDevice>(profile->devices[input.input.wiiButton.deviceid])));
    case proto_Input_vtechExpander_tag:
        if (profile->devices.find(input.input.vtechExpander.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new VTechExpanderInput(input.input.vtechExpander, std::static_pointer_cast<VTechGuitarIOExpanderDevice>(profile->devices[input.input.vtechExpander.deviceid])));
    case proto_Input_crkd_tag:
        if (profile->devices.find(input.input.crkd.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new CrkdButtonInput(input.input.crkd, std::static_pointer_cast<CrkdDevice>(profile->devices[input.input.crkd.deviceid])));
    case proto_Input_crkdDrum_tag:
        if (profile->devices.find(input.input.crkdDrum.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new CrkdDrumInput(input.input.crkdDrum, std::static_pointer_cast<CrkdDrumDevice>(profile->devices[input.input.crkdDrum.deviceid])));
    case proto_Input_gpio_tag:
        return std::unique_ptr<Input>(new GPIOInput(input.input.gpio));
    case proto_Input_ads1115_tag:
        if (profile->devices.find(input.input.ads1115.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new ADS1115Input(input.input.ads1115, std::static_pointer_cast<ADS1115Device>(profile->devices[input.input.ads1115.deviceid])));
    case proto_Input_multiplexer_tag:
        if (profile->devices.find(input.input.multiplexer.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MultiplexerInput(input.input.multiplexer, std::static_pointer_cast<MultiplexerDevice>(profile->devices[input.input.multiplexer.deviceid])));
    case proto_Input_accelerometer_tag:
        if (profile->devices.find(input.input.accelerometer.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new AccelerometerInput(input.input.accelerometer, std::static_pointer_cast<AccelerometerDevice>(profile->devices[input.input.accelerometer.deviceid])));
    case proto_Input_gh5Neck_tag:
        if (profile->devices.find(input.input.gh5Neck.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new Gh5ButtonInput(input.input.gh5Neck, std::static_pointer_cast<GH5NeckDevice>(profile->devices[input.input.gh5Neck.deviceid])));
    case proto_Input_fixed_tag:
        return std::unique_ptr<Input>(new FixedInput(input.input.fixed));
    case proto_Input_ps2Axis_tag:
        if (profile->devices.find(input.input.ps2Axis.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new PS2AxisInput(input.input.ps2Axis, std::static_pointer_cast<PS2Device>(profile->devices[input.input.ps2Axis.deviceid])));
    case proto_Input_ps2Button_tag:
        if (profile->devices.find(input.input.ps2Button.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new PS2ButtonInput(input.input.ps2Button, std::static_pointer_cast<PS2Device>(profile->devices[input.input.ps2Button.deviceid])));
    case proto_Input_mpr121_tag:
        if (profile->devices.find(input.input.mpr121.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MPR121Input(input.input.mpr121, std::static_pointer_cast<MPR121Device>(profile->devices[input.input.mpr121.deviceid])));
    case proto_Input_midiNote_tag:
        if (profile->devices.find(input.input.midiNote.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MidiNoteInput(input.input.midiNote, std::static_pointer_cast<MidiDeviceWithChannel>(profile->midiDevices[input.input.midiNote.deviceid])));
    case proto_Input_midiControlChange_tag:
        if (profile->devices.find(input.input.midiControlChange.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MidiControlChangeInput(input.input.midiControlChange, std::static_pointer_cast<MidiDeviceWithChannel>(profile->midiDevices[input.input.midiControlChange.deviceid])));
    case proto_Input_midiPitchBend_tag:
        if (profile->devices.find(input.input.midiPitchBend.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MidiPitchBendInput(input.input.midiPitchBend, std::static_pointer_cast<MidiDeviceWithChannel>(profile->midiDevices[input.input.midiPitchBend.deviceid])));
    case proto_Input_midiProGuitarButton_tag:
        if (profile->devices.find(input.input.midiProGuitarButton.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MidiProGuitarButtonInput(input.input.midiProGuitarButton, std::static_pointer_cast<ProGuitarMidiDevice>(profile->midiDevices[input.input.midiProGuitarButton.deviceid])));
    case proto_Input_midiProGuitarAxis_tag:
        if (profile->devices.find(input.input.midiProGuitarAxis.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MidiProGuitarAxisInput(input.input.midiProGuitarAxis, std::static_pointer_cast<ProGuitarMidiDevice>(profile->midiDevices[input.input.midiProGuitarAxis.deviceid])));
    case proto_Input_protarNeckButton_tag:
        if (profile->devices.find(input.input.protarNeckButton.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new ProtarNeckButtonInput(input.input.protarNeckButton, std::static_pointer_cast<ProtarNeckDevice>(profile->devices[input.input.protarNeckButton.deviceid])));
    case proto_Input_protarNeckAxis_tag:
        if (profile->devices.find(input.input.protarNeckAxis.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new ProtarNeckAxisInput(input.input.protarNeckAxis, std::static_pointer_cast<ProtarNeckDevice>(profile->devices[input.input.protarNeckAxis.deviceid])));
    case proto_Input_mouseAxis_tag:
        if (profile->devices.find(input.input.mouseAxis.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MouseAxisInput(input.input.mouseAxis, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.mouseAxis.deviceid])));
    case proto_Input_mouseButton_tag:
        if (profile->devices.find(input.input.mouseButton.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new MouseButtonInput(input.input.mouseButton, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.mouseButton.deviceid])));
    case proto_Input_key_tag:
        if (profile->devices.find(input.input.key.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new KeyboardKeyInput(input.input.key, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.key.deviceid])));
    case proto_Input_usbButton_tag:
        if (profile->devices.find(input.input.usbButton.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new USBButtonInput(input.input.usbButton, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.usbButton.deviceid])));
    case proto_Input_usbAxis_tag:
        if (profile->devices.find(input.input.usbAxis.deviceid) == profile->devices.end())
        {
            return nullptr;
        }
        return std::unique_ptr<Input>(new USBAxisInput(input.input.usbAxis, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.usbAxis.deviceid])));
    case proto_Input_held_tag:
    case proto_Input_cycle_tag:
    case proto_Input_toggle_tag:
    case proto_Input_shortcut_tag:
    case 0:
    {
        auto ret = last_special;
        last_special = nullptr;
        last_shortcut = nullptr;
        return std::unique_ptr<Input>(ret);
    }
    default:
        return nullptr;
    }
}
bool load_input_dev(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool load_shortcut_input(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    proto_Input input;
    if (!pb_decode(stream, proto_Input_fields, &input))
    {
        printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    auto inputPtr = make_input(input, profile, stream);
    if (!inputPtr)
    {
        return true;
    }
    last_shortcut->inputs.push_back(std::move(inputPtr));
    printf("shortcut added: %d\r\n", last_shortcut->inputs.size());
    return true;
}
bool load_shortcut(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("found shortcut!\r\n");
    last_shortcut = new ShortcutInput();
    last_special = last_shortcut;
    proto_ShortcutInput input;
    input.inputs.funcs.decode = &load_shortcut_input;
    if (!pb_decode(stream, proto_ShortcutInput_fields, &input))
    {
        printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    printf("loaded shortcut\r\n");
    return true;
}
bool load_held(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("found held!\r\n");
    auto last_held = new HeldInput();
    last_special = last_held;
    proto_HeldInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    if (!pb_decode(stream, proto_HeldInput_fields, &input))
    {
        printf("couldnt decode held input?\r\n");
        return false;
    }
    last_held->load(input, make_input(input.input, profile, stream));
    return true;
}
bool load_cycle(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("found cycle! %p\r\n", profile.get());
    auto last_cycle = new CycleInput();
    last_special = last_cycle;
    proto_CycleInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    if (!pb_decode(stream, proto_CycleInput_fields, &input))
    {
        printf("couldnt decode cycle input?\r\n");
        return false;
    }

    printf("check %d %d\r\n", input.deviceid, profile->devices.size());
    if (profile->devices.find(input.deviceid) == profile->devices.end())
    {
        printf("why tho\r\n");
        return true;
    }
    printf("loading cycle\r\n");
    last_cycle->load(input, std::static_pointer_cast<CycleDevice>(profile->devices[input.deviceid]), input.has_input ? make_input(input.input, profile, stream) : nullptr, input.has_inputReverse ? make_input(input.inputReverse, profile, stream) : nullptr);
    printf("loaded cycle\r\n");
    return true;
}
bool load_toggle(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("found toggle! %p\r\n", profile.get());
    auto last_toggle = new ToggleInput();
    last_special = last_toggle;
    proto_ToggleInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    if (!pb_decode(stream, proto_ToggleInput_fields, &input))
    {
        printf("couldnt decode toggle input?\r\n");
        return false;
    }

    printf("check %d %d\r\n", input.deviceid, profile->devices.size());
    if (profile->devices.find(input.deviceid) == profile->devices.end())
    {
        printf("why tho\r\n");
        return true;
    }
    printf("loading toggle\r\n");
    last_toggle->load(input, std::static_pointer_cast<ToggleDevice>(profile->devices[input.deviceid]), input.has_input ? make_input(input.input, profile, stream) : nullptr);
    printf("loaded toggle\r\n");
    return true;
}

bool load_input_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("input_dev: %d %p\r\n", field->tag, profile.get());

    if (field->tag == proto_Input_cycle_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_cycle;
    }
    if (field->tag == proto_Input_toggle_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_toggle;
    }
    if (field->tag == proto_Input_held_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_held;
    }
    if (field->tag == proto_Input_shortcut_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_shortcut;
    }
    return true;
}
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("load_mapping: %p\r\n", profile.get());
    proto_Mapping mapping;
    mapping.input.cb_input.funcs.decode = load_input_dev;
    pb_decode(stream, proto_Mapping_fields, &mapping);
    std::unique_ptr<Input> input = make_input(mapping.input, profile, stream);
    if (!input)
    {
        return true;
    }
    size_t mapping_id = profile->mappings.size();
    switch (mapping.which_mapping)
    {
    case proto_Mapping_gamepadAxis_tag:
        profile->mappings.emplace_back(new GamepadAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghAxis_tag:
        profile->mappings.emplace_back(new GuitarHeroGuitarAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghButton_tag:
        profile->mappings.emplace_back(new GuitarHeroGuitarButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_rbAxis_tag:
        profile->mappings.emplace_back(new RockBandGuitarAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_rbButton_tag:
        profile->mappings.emplace_back(new RockBandGuitarButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_gamepadButton_tag:
        profile->mappings.emplace_back(new GamepadButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghDrumButton_tag:
        profile->mappings.emplace_back(new GuitarHeroDrumsButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghDrumAxis_tag:
        profile->mappings.emplace_back(new GuitarHeroDrumsAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_rbDrumButton_tag:
        profile->mappings.emplace_back(new RockBandDrumsButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_rbDrumAxis_tag:
        profile->mappings.emplace_back(new RockBandDrumsAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghlButton_tag:
        profile->mappings.emplace_back(new LiveGuitarButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghlAxis_tag:
        profile->mappings.emplace_back(new LiveGuitarAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_proButton_tag:
        profile->mappings.emplace_back(new ProGuitarButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_proAxis_tag:
        profile->mappings.emplace_back(new ProGuitarAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_djhButton_tag:
        profile->mappings.emplace_back(new DJHTurntableButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_djhAxis_tag:
        profile->mappings.emplace_back(new DJHTurntableAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_djMaxButton_tag:
        profile->mappings.emplace_back(new DJMaxTurntableButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_djMaxAxis_tag:
        profile->mappings.emplace_back(new DJMaxTurntableAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_divaButton_tag:
        profile->mappings.emplace_back(new ProjectDivaButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_divaAxis_tag:
        profile->mappings.emplace_back(new ProjectDivaAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_gfAxis_tag:
        profile->mappings.emplace_back(new GuitarFreaksAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_gfButton_tag:
        profile->mappings.emplace_back(new GuitarFreaksButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghaAxis_tag:
        profile->mappings.emplace_back(new GuitarHeroArcadeAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_ghaButton_tag:
        profile->mappings.emplace_back(new GuitarHeroArcadeButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_mouseAxis_tag:
        profile->mappings.emplace_back(new MouseAxisMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_mouseButton_tag:
        profile->mappings.emplace_back(new MouseButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    case proto_Mapping_keycode_tag:
        profile->mappings.emplace_back(new KeyboardButtonMapping(mapping, std::move(input), mapping_id, profile->profile_id));
        break;
    }
    return true;
}

bool load_assignment_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("load_assignment_dev: %d %p\r\n", field->tag, profile.get());
    proto_ProfileAssignmentInfo *info = (proto_ProfileAssignmentInfo *)field->message;
    if (field->tag == proto_ProfileAssignmentInfo_input_tag)
    {
        info->assignment.input.input.cb_input.funcs.decode = load_input_dev;
    }
    if (field->tag == proto_ProfileAssignmentInfo_inputAnyTime_tag)
    {
        info->assignment.inputAnyTime.input.cb_input.funcs.decode = load_input_dev;
    }
    return true;
}
bool load_assignment_info(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("load assignment info before 2 %p\r\n", profile);
    fflush(stdout);
    printf("load_assignment_info: %p\r\n", profile.get());
    fflush(stdout);
    auto &list = profile->triggers.back();
    proto_ProfileAssignmentInfo assignment;
    assignment.cb_assignment.funcs.decode = load_assignment_dev;
    pb_decode(stream, proto_ProfileAssignmentInfo_fields, &assignment);
    switch (assignment.which_assignment)
    {
    case proto_ProfileAssignmentInfo_input_tag:
    {

        std::unique_ptr<Input> input = make_input(assignment.assignment.input.input, profile, stream);
        if (input == nullptr)
        {
            return true;
        }
        list->triggers.emplace_back(new InputActivationTrigger(false, assignment.assignment.input, std::move(input), profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    }
    case proto_ProfileAssignmentInfo_inputAnyTime_tag:
    {

        std::unique_ptr<Input> input = make_input(assignment.assignment.inputAnyTime.input, profile, stream);
        if (input == nullptr)
        {
            return true;
        }
        printf("input any time! %p\r\n", input.get());
        list->triggers.emplace_back(new InputActivationTrigger(true, assignment.assignment.inputAnyTime, std::move(input), profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    }
    case proto_ProfileAssignmentInfo_consoleType_tag:
        list->triggers.emplace_back(new UsbModeActivationTrigger(assignment.assignment.consoleType, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_wiiExt_tag:
        list->triggers.emplace_back(new WiiExtTypeActivationTrigger(assignment.assignment.wiiExt, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_ps2Cnt_tag:
        list->triggers.emplace_back(new PS2ControllerTypeActivationTrigger(assignment.assignment.ps2Cnt, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_usbType_tag:
        list->triggers.emplace_back(new UsbTypeActivationTrigger(assignment.assignment.usbType, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_usbDevice_tag:
        list->triggers.emplace_back(new SpecificUsbDeviceActivationTrigger(assignment.assignment.usbDevice, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_catchall_tag:
        list->triggers.emplace_back(new CatchAllActivationTrigger(assignment.assignment.catchall, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_midiChannel_tag:
        list->triggers.emplace_back(new MidiChannelActivationTrigger(assignment.assignment.midiChannel, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_ps2Emulation_tag:
        list->triggers.emplace_back(new PS2ControllerEmulationActivationTrigger(assignment.assignment.ps2Emulation, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_wiiEmulation_tag:
        list->triggers.emplace_back(new WiiExtensionEmulationActivationTrigger(assignment.assignment.wiiEmulation, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_bluetooth_tag:
        list->triggers.emplace_back(new BluetoothModeActivationTrigger(assignment.assignment.bluetooth, profile->profile_id, list->triggers.size(), profile->triggers.size() - 1));
        break;
    case proto_ProfileAssignmentInfo_copilotProfile_tag:
        // TODO: how do we handle this
        break;
    }
    return true;
}

bool load_assignments(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("load_assignments: %p\r\n", profile.get());
    auto list = new ActivationTriggerList();
    profile->triggers.emplace_back(list);
    proto_ProfileAssignment proto_assignment;
    proto_assignment.assignments.funcs.decode = &load_assignment_info;
    printf("load_assignments start?\r\n");
    pb_decode(stream, proto_ProfileAssignment_fields, &proto_assignment);
    printf("load_assignments done?\r\n");
    list->validate(true, false, false);
    return true;
}
bool load_uid(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("load_uid: %p\r\n", profile.get());
    uint64_t value;
    if (!pb_decode_varint(stream, &value))
        return false;
    profile->profile_id = value;
    all_profiles[profile->profile_id] = profile;
    return true;
}
bool load_leds(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    printf("load_leds: %p\r\n", profile.get());
    std::unique_ptr<LedMappingDevice> device;
    proto_Led proto_led;
    proto_led.mapping.led.inputMapping.input.cb_input.funcs.decode = load_input_dev;
    pb_decode(stream, proto_Led_fields, &proto_led);
    printf("load led%d %d\r\n", profile->leds.size(), proto_led.device.which_device);
    switch (proto_led.device.which_device)
    {
    case proto_LedDevice_rgb_tag:
        device = std::make_unique<RgbLedDevice>(proto_led.device.device.rgb, std::static_pointer_cast<LedDevice>(profile->devices[proto_led.device.device.rgb.deviceId]));
        printf("dev rgb%d\r\n", profile->leds.size());
        break;
    case proto_LedDevice_gpio_tag:
        device = std::make_unique<GpioLedDevice>(proto_led.device.device.gpio);
        printf("dev gpio%d\r\n", profile->leds.size());
        break;
    case proto_LedDevice_dmx_tag:
        device = std::make_unique<DMXLedDevice>(proto_led.device.device.dmx, std::static_pointer_cast<DMXDevice>(profile->devices[proto_led.device.device.dmx.deviceId]));
        printf("dev dmx%d\r\n", profile->leds.size());
        break;
    case proto_LedDevice_stp16_tag:
        device = std::make_unique<STP16CPCLedDevice>(proto_led.device.device.stp16, std::static_pointer_cast<STP16CPCDevice>(profile->devices[proto_led.device.device.stp16.deviceId]));
        printf("dev stp16%d\r\n", profile->leds.size());
        break;
    case proto_LedDevice_vtechExpander_tag:
        device = std::make_unique<VTechGuitarIoExpanderLedDevice>(proto_led.device.device.vtechExpander, std::static_pointer_cast<VTechGuitarIOExpanderDevice>(profile->devices[proto_led.device.device.vtechExpander.deviceId]));
        printf("dev stp16%d\r\n", profile->leds.size());
        break;
    }
    if (!device)
    {
        printf("cant load led%d\r\n", profile->leds.size());
        return false;
    }
    switch (proto_led.mapping.which_led)
    {
    case proto_LedMapping_inputMapping_tag:
        profile->leds.emplace_back(new InputLedMapping(std::move(device), proto_led.mapping.led.inputMapping, make_input(proto_led.mapping.led.inputMapping.input, profile, stream), profile->profile_id, profile->leds.size()));
        printf("loaded led input%d\r\n", profile->leds.size());
        return true;
    case proto_LedMapping_staticMapping_tag:
        profile->leds.emplace_back(new StaticLedMapping(std::move(device), proto_led.mapping.led.staticMapping, profile->profile_id, profile->leds.size()));
        printf("loaded led static%d\r\n", profile->leds.size());
        return true;
    case proto_LedMapping_patternMapping_tag:
        profile->leds.emplace_back(new PatternLedMapping(std::move(device), proto_led.mapping.led.patternMapping, profile->profile_id, profile->leds.size()));
        printf("loaded led pattern%d\r\n", profile->leds.size());
        return true;
    }
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = std::make_shared<Profile>();
    for (auto &device : active_devices)
    {
        profile->devices.emplace(device->m_id, device);
        printf("load device: %p %p %d\r\n", profile.get(), device.get(), device->m_id);
    }
    working_profile = profile;
    proto_Profile proto_profile;
    memset(&proto_profile, 0, sizeof(proto_profile));
    proto_profile.assignments.funcs.decode = &load_assignments;
    proto_profile.mappings.funcs.decode = &load_mapping;
    proto_profile.uid.funcs.decode = &load_uid;
    proto_profile.leds.funcs.decode = &load_leds;
    pb_decode(stream, proto_Profile_fields, &proto_profile);
    memcpy(profile->name, proto_profile.name, sizeof(profile->name));
    profile->subtype = proto_profile.deviceToEmulate;
    profile->xinput_on_windows = proto_profile.has_xinputOnWindows && proto_profile.xinputOnWindows;
    profile->invert_y_axis_hid = proto_profile.has_invertYAxisHid && proto_profile.invertYAxisHid;
    profile->supports_ps4 = proto_profile.has_ps4OrPs5Mode && proto_profile.ps4OrPs5Mode;
    printf("profile loaded: %d %d %d\r\n", profile->profile_id, profile->xinput_on_windows, profile->invert_y_axis_hid);
    std::shared_ptr<UsbDevice> usbInstance;
    std::shared_ptr<BTGamepadDevice> btGamepadInstance;
    for (auto &list : profile->triggers)
    {
        if (list->validate(true, false, false))
        {
            int assignedDevices = list->assignedDevices();
            printf("profile assigned! %d\r\n", profile->profile_id);
            if ((assignedDevices & ProfileAssignMask_AssignBluetoothGamepad) && !(seenMasks & ProfileAssignMask_AssignBluetoothGamepad) && isPicoW)
            {
                btGamepadInstance = std::make_shared<BTGamepadDevice>();
                instances.push_back(btGamepadInstance);
                btGamepadInstance->profiles.push_back(profile);
                btGamepadInstance->subtype = profile->subtype;
                btGamepadInstance->xinput_on_windows = profile->xinput_on_windows;
                btGamepadInstance->invert_y_axis_hid = profile->invert_y_axis_hid;
                btGamepadInstance->supports_ps4 = profile->supports_ps4;
                active_instances.push_back(btGamepadInstance);
                btGamepadInstance->initialize();
                active_profiles.insert(profile->profile_id);
                seenMasks |= ProfileAssignMask_AssignBluetoothGamepad;
                printf("assigned bluetooth!\r\n");
            }
            if (!usbInstance && (assignedDevices & ProfileAssignMask_AssignUsb))
            {
                switch (mode)
                {
                case ModeHid:
                    usbInstance = std::make_shared<HIDGamepadDevice>();
                    break;
                case ModeOgXbox:
                    usbInstance = std::make_shared<OGXboxGamepadDevice>();
                    break;
                case ModeXbox360:
                    usbInstance = std::make_shared<XInputGamepadDevice>();
                    break;
                case ModeXboxOne:
                    usbInstance = std::make_shared<XboxOneGamepadDevice>();
                    break;
                case ModeWiiRb:
                    // wii rb is the same as ps3 but different ids
                    usbInstance = std::make_shared<PS3GamepadDevice>(true);
                    break;
                case ModePs3:
                    usbInstance = std::make_shared<PS3GamepadDevice>(false);
                    break;
                case ModePs4:
                    usbInstance = std::make_shared<PS4GamepadDevice>();
                    break;
                case ModePs5:
                    usbInstance = std::make_shared<PS5GamepadDevice>();
                    break;
                case ModeSwitch:
                    usbInstance = std::make_shared<SwitchGamepadDevice>();
                    break;
                case ModeGuitarHeroArcade:
                    usbInstance = std::make_shared<GHArcadeGamepadDevice>();
                    break;
                }
                instances.push_back(usbInstance);
                usbInstance->profiles.push_back(profile);
                usbInstance->interface_id = active_instances.size();
                usbInstance->subtype = profile->subtype;
                usbInstance->xinput_on_windows = profile->xinput_on_windows;
                usbInstance->invert_y_axis_hid = profile->invert_y_axis_hid;
                usbInstance->supports_ps4 = profile->supports_ps4;
                active_instances.push_back(usbInstance);
                usb_instances[usbInstance->interface_id] = usbInstance;
                usbInstance->initialize();
                active_profiles.insert(profile->profile_id);
                printf("instance: %d\r\n", usbInstance->interface_id);
            }
            break;
        }
    }
    return true;
}
struct ConfigFooter
{
    uint32_t dataSize;
    uint32_t dataCrc;
    uint32_t mainSize;
    uint32_t auxSize;
    uint32_t magic;
    uint32_t currentProfile;

    bool operator==(const ConfigFooter &other) const
    {
        return dataSize == other.dataSize &&
               dataCrc == other.dataCrc &&
               magic == other.magic;
    }
};

static const uint32_t FOOTER_MAGIC = 0xd2f1e365;
bool save_empty()
{

    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));

    footer->dataSize = 0;
    footer->mainSize = 0;
    footer->auxSize = 0;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, 0);
    footer->magic = FOOTER_MAGIC;
    footer->currentProfile = 0;
    EEPROM.commit_now();
    return true;
}

bool decode_cycle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_CyclingInputState proto_cycle;
    auto ret = pb_decode(stream, proto_CyclingInputState_fields, &proto_cycle);
    cycle_input_states[proto_cycle.id] = proto_cycle.state;
    return ret;
}
bool encode_cycle_input_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_CyclingInputState proto_cycle;
    for (auto &state : cycle_input_states)
    {
        proto_cycle.id = state.first;
        proto_cycle.state = state.second;
        pb_encode_tag_for_field(stream, field);
        pb_encode_submessage(stream, proto_CyclingInputState_fields, &proto_cycle);
    }
    return true;
}
bool decode_toggle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_ToggleInputState proto_toggle;
    auto ret = pb_decode(stream, proto_ToggleInputState_fields, &proto_toggle);
    toggle_input_states[proto_toggle.id] = proto_toggle.state;
    return ret;
}
bool encode_toggle_input_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_ToggleInputState proto_toggle;
    for (auto &state : toggle_input_states)
    {
        proto_toggle.id = state.first;
        proto_toggle.state = state.second;
        pb_encode_tag_for_field(stream, field);
        pb_encode_submessage(stream, proto_ToggleInputState_fields, &proto_toggle);
    }
    return true;
}
bool inner_load(const uint32_t currentProfile, const uint8_t *dataPtr, uint32_t size, uint32_t mainSize, uint32_t auxSize)
{

    proto_Config config proto_Config_init_zero;
    printf("inner_load\r\n");
    cycle_input_states.clear();
    pb_istream_t auxInputStream = pb_istream_from_buffer(dataPtr + mainSize, auxSize);
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.decode = decode_cycle_input_states;
    block.toggleStates.funcs.decode = decode_toggle_input_states;
    pb_decode(&auxInputStream, proto_AuxConfigBlock_fields, &block);
    // We are now sufficiently confident that the data is valid so we run the deserialization
    // load just the current profile to begin with
    pb_istream_t inputStream = pb_istream_from_buffer(dataPtr, mainSize);
    assignable_devices.clear();

    config.devices.funcs.decode = &load_device;
    config.profiles.funcs.decode = &load_profile;
    instances.clear();
    active_instances.clear();
    seenMasks = 0;
    for (size_t i = 0; i < TU_ARRAY_SIZE(usb_instances); i++)
    {
        usb_instances[i].reset();
    }
    for (size_t i = 0; i < TU_ARRAY_SIZE(usb_instances_by_epin); i++)
    {
        usb_instances_by_epin[i].reset();
    }
    for (size_t i = 0; i < TU_ARRAY_SIZE(usb_instances_by_epout); i++)
    {
        usb_instances_by_epout[i].reset();
    }
    active_devices.clear();
    for (auto &device : root_devices)
    {
        device.second->still_connected = false;
    }
    if (fullReload)
    {
        root_devices.clear();
    }
    active_profiles.clear();
    all_profiles.clear();
    UsbDevice::reset_ep();
    switch (mode)
    {
    case ModeOgXbox:
    case ModeXboxOne:
    case ModeWiiRb:
    case ModePs3:
    case ModePs4:
    case ModePs5:
    case ModeSwitch:
        break;
    case ModeHid:
    case ModeXbox360:
    {
        auto secDevice = std::make_shared<XInputSecurityDevice>();
        secDevice->interface_id = instances.size();
        instances.push_back(secDevice);
        active_instances.push_back(secDevice);
        usb_instances[secDevice->interface_id] = secDevice;
        secDevice->initialize();
        break;
    }
    case ModeGuitarHeroArcade:
    {
        auto venDevice = std::make_shared<GHArcadeVendorDevice>();
        venDevice->interface_id = instances.size();
        instances.push_back(std::move(venDevice));
        active_instances.push_back(venDevice);
        usb_instances[venDevice->interface_id] = venDevice;
        venDevice->initialize();
        break;
    }
    }
    auto ret = pb_decode(&inputStream, proto_Config_fields, &config);
    if (active_instances.empty() || mode == ModeHid || mode == ModeXbox360)
    {
        auto confDevice2 = HIDConfigDevice::instance;
        confDevice2->interface_id = instances.size();
        instances.push_back(confDevice2);
        active_instances.push_back(confDevice2);
        usb_instances[confDevice2->interface_id] = confDevice2;
        confDevice2->initialize();
    }
    auto iter = root_devices.begin();
    auto endIter = root_devices.end();
    for (; iter != endIter;)
    {
        if (!iter->second->still_connected)
        {
            iter = root_devices.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    return ret;
}
uint32_t copy_config_info(uint8_t *buffer)
{
    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(flashEnd - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    info.dataCrc = footer.dataCrc;
    info.dataSize = footer.dataSize;
    info.magic = footer.magic;
    info.mainSize = footer.mainSize;
    info.auxSize = footer.auxSize;
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 64);
    if (!pb_encode(&outputStream, proto_ConfigInfo_fields, &info))
    {
        return 0;
    }
    return outputStream.bytes_written;
}

void reload()
{
    printf("reload called\r\n");
    fullReload = false;
    reinit = true;
}

void update_aux_cycle(uint32_t id, uint32_t state)
{
    printf("update aux: %d %d %d\r\n", id, state, reinit);
    if (reinit)
    {
        return;
    }
    cycle_input_states[id] = state;
    // Aux region contains data that the firmware can update itself
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    // move data to start of cache to make modifying easier
    memmove(EEPROM.writeCache, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, footer->dataSize);
    pb_ostream_t auxOutputStream = pb_ostream_from_buffer(EEPROM.writeCache + footer->mainSize, EEPROM_SIZE_BYTES - footer->mainSize - sizeof(ConfigFooter));
    proto_AuxConfigBlock block;
    block.states.funcs.encode = encode_cycle_input_states;
    block.toggleStates.funcs.encode = encode_toggle_input_states;
    pb_encode(&auxOutputStream, proto_AuxConfigBlock_fields, &block);
    footer->auxSize = auxOutputStream.bytes_written;
    footer->dataSize = footer->mainSize + footer->auxSize;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, footer->dataSize);
    // Move the encoded data to end where it should be
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, EEPROM.writeCache, footer->dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize);
    EEPROM.commit_now();
}

void update_aux_toggle(uint32_t id, bool state)
{
    printf("update aux: %d %d %d\r\n", id, state, reinit);
    if (reinit)
    {
        return;
    }
    cycle_input_states[id] = state;
    // Aux region contains data that the firmware can update itself
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    // move data to start of cache to make modifying easier
    memmove(EEPROM.writeCache, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, footer->dataSize);
    pb_ostream_t auxOutputStream = pb_ostream_from_buffer(EEPROM.writeCache + footer->mainSize, EEPROM_SIZE_BYTES - footer->mainSize - sizeof(ConfigFooter));
    proto_AuxConfigBlock block;
    block.states.funcs.encode = encode_cycle_input_states;
    block.toggleStates.funcs.encode = encode_toggle_input_states;
    pb_encode(&auxOutputStream, proto_AuxConfigBlock_fields, &block);
    footer->auxSize = auxOutputStream.bytes_written;
    footer->dataSize = footer->mainSize + footer->auxSize;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, footer->dataSize);
    // Move the encoded data to end where it should be
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, EEPROM.writeCache, footer->dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize);
    EEPROM.commit_now();
}

bool write_config_info(const uint8_t *buffer, uint16_t bufsize)
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
    if (!pb_decode_delimited(&inputStream, proto_ConfigInfo_fields, &info))
    {
        printf("Didn't decode info?\r\n");
        return false;
    }
    footer->dataCrc = info.dataCrc;
    footer->dataSize = info.dataSize;
    footer->magic = info.magic;
    footer->mainSize = info.mainSize;
    footer->auxSize = info.auxSize;
    return true;
}

bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start)
{
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    if (bufsize + start > footer.dataSize)
    {
        bufsize = footer.dataSize - start;
    }
    memcpy(EEPROM.writeCache + start, buffer, bufsize);
    if (start + bufsize < footer.dataSize)
    {
        // printf("writing up to: %d < %d\r\n", start + bufsize, footer.dataSize);
        working = true;
        return true;
    }
    uint32_t crc = CRC32::calculate(EEPROM.writeCache, footer.dataSize);
    if (crc != footer.dataCrc)
    {
        printf("Crc didnt match after writing? %d\r\n", footer.dataCrc);
        return false;
    }
    printf("Everything matched, saving!\r\n");
    // Move the encoded data in memory down to the footer
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, EEPROM.writeCache, footer.dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize);
    EEPROM.commit();
    inner_load(footer.currentProfile, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, footer.dataSize, footer.mainSize, footer.auxSize);
    working = false;
    return true;
}

uint32_t copy_config(uint8_t *buffer, uint32_t start)
{
    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(flashEnd - sizeof(ConfigFooter));

    // Check for presence of magic value
    if (footer.magic != FOOTER_MAGIC)
    {
        return 0;
    }

    // Check if dataSize exceeds the reserved space
    if (footer.dataSize + sizeof(ConfigFooter) > EEPROM_SIZE_BYTES)
    {
        return 0;
    }

    const uint8_t *dataPtr = flashEnd - sizeof(ConfigFooter) - footer.dataSize;

    // Verify CRC32 hash
    if (CRC32::calculate(dataPtr, footer.dataSize) != footer.dataCrc)
    {
        return 0;
    }
    const uint32_t remaining = footer.dataSize - start;
    const uint32_t size = remaining > 63 ? 63 : remaining;
    memcpy(buffer, dataPtr + start, size);
    return size;
}
bool load()
{
    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(flashEnd - sizeof(ConfigFooter));

    // Check for presence of magic value
    if (footer.magic != FOOTER_MAGIC)
    {
        printf("footer wrong %x != %x\r\n", footer.magic, FOOTER_MAGIC);
        return false;
    }

    // Check if dataSize exceeds the reserved space
    if (footer.dataSize + sizeof(ConfigFooter) > EEPROM_SIZE_BYTES)
    {
        return false;
    }

    const uint8_t *dataPtr = flashEnd - sizeof(ConfigFooter) - footer.dataSize;

    // Verify CRC32 hash
    if (CRC32::calculate(dataPtr, footer.dataSize) != footer.dataCrc)
    {
        return false;
    }

    return inner_load(footer.currentProfile, dataPtr, footer.dataSize, footer.mainSize, footer.auxSize);
}

void first_load()
{
    UsbDevice::reset_ep();
    active_devices.clear();
    root_devices.clear();
    instances.clear();
    active_instances.clear();
    for (size_t i = 0; i < TU_ARRAY_SIZE(usb_instances); i++)
    {
        usb_instances[i] = std::shared_ptr<UsbDevice>();
    }
    for (size_t i = 0; i < TU_ARRAY_SIZE(usb_instances_by_epin); i++)
    {
        usb_instances_by_epin[i] = std::shared_ptr<UsbDevice>();
    }
    for (size_t i = 0; i < TU_ARRAY_SIZE(usb_instances_by_epout); i++)
    {
        usb_instances_by_epout[i] = std::shared_ptr<UsbDevice>();
    }
    all_profiles.clear();
    auth_devices.clear();
    auto confDevice = std::make_shared<HIDConfigDevice>();
    confDevice->interface_id = instances.size();
    confDevice->initialize();
    instances.push_back(confDevice);
    active_instances.push_back(confDevice);
    usb_instances[confDevice->interface_id] = confDevice;
}