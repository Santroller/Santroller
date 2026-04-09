#include "config.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "input/fixed.hpp"
#include "input/wii.hpp"
#include "input/mpr121.hpp"
#include "input/ps2.hpp"
#include "input/midi.hpp"
#include "input/usb.hpp"
#include "input/crkd.hpp"
#include "input/shortcut.hpp"
#include "input/ads1115.hpp"
#include "input/accelerometer.hpp"
#include "input/multiplexer.hpp"
#include "input/gh5.hpp"
#include "devices/base.hpp"
#include "devices/accelerometer.hpp"
#include "devices/multiplexer.hpp"
#include "devices/wii.hpp"
#include "devices/bhdrum.hpp"
#include "devices/crazyneck.hpp"
#include "devices/debug.hpp"
#include "devices/djh.hpp"
#include "devices/crkd.hpp"
#include "devices/ads1115.hpp"
#include "devices/gh5neck.hpp"
#include "devices/usb.hpp"
#include "devices/ps2.hpp"
#include "devices/max1704x.hpp"
#include "devices/mpr121.hpp"
#include "devices/ws2812.hpp"
#include "devices/apa102.hpp"
#include "devices/midiserial.hpp"
#include "devices/stp16cpc.hpp"
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
// devices that have not yet been assigned to a profile
std::vector<std::shared_ptr<Device>> assignable_devices;
std::vector<std::shared_ptr<UsbHostInterface>> assignable_usb_devices;
std::map<ConsoleMode, std::shared_ptr<UsbHostInterface>> auth_devices;
std::unordered_map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances;
std::unordered_map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances_by_epnum;
ConsoleMode mode = ModeHid;
ConsoleMode newMode = mode;
bool working = false;
bool loadedAny = false;

bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Device device;
    uint16_t *dev_id = (uint16_t *)*arg;
    pb_decode(stream, proto_Device_fields, &device);
    switch (device.which_device)
    {
    case proto_Device_accelerometer_tag:
        active_devices.emplace_back(new AccelerometerDevice(device.device.accelerometer, *dev_id));
        break;
    case proto_Device_crkdNeck_tag:
        active_devices.emplace_back(new CrkdDevice(device.device.crkdNeck, *dev_id));
        break;
    case proto_Device_wii_tag:
    {
        auto &wiiDev = active_devices.emplace_back(new WiiDevice(device.device.wii, *dev_id));
        wiiDev->rescan(true);
        break;
    }
    case proto_Device_psx_tag:
    {
        for (int i = A; i <= D; i++)
        {
            auto &ps2Dev = active_devices.emplace_back(new PS2Device(device.device.psx, *dev_id, (MultitapPort)i));
            ps2Dev->rescan(true);
        }
        break;
    }
    case proto_Device_bhDrum_tag:
        active_devices.emplace_back(new BandHeroDrumDevice(device.device.bhDrum, *dev_id));
        break;
    case proto_Device_crazyGuitarNeck_tag:
        active_devices.emplace_back(new CrazyGuitarNeckDevice(device.device.crazyGuitarNeck, *dev_id));
        break;
    case proto_Device_djhTurntable_tag:
        active_devices.emplace_back(new DjHeroTurntableDevice(device.device.djhTurntable, *dev_id));
        break;
    case proto_Device_gh5Neck_tag:
        active_devices.emplace_back(new GH5NeckDevice(device.device.gh5Neck, *dev_id));
        break;
    case proto_Device_max1704x_tag:
        active_devices.emplace_back(new Max1704XDevice(device.device.max1704x, *dev_id));
        break;
    case proto_Device_mpr121_tag:
        active_devices.emplace_back(new MPR121Device(device.device.mpr121, *dev_id));
        break;
    case proto_Device_usbHost_tag:
        active_devices.emplace_back(new USBHostHardwareDevice(device.device.usbHost, *dev_id));
        break;
    case proto_Device_ads1115_tag:
        active_devices.emplace_back(new ADS1115Device(device.device.ads1115, *dev_id));
        break;
    case proto_Device_debug_tag:
        active_devices.emplace_back(new DebugDevice(device.device.debug, *dev_id));
        break;
    case proto_Device_midiSerial_tag:
        active_devices.emplace_back(new MidiSerialDevice(device.device.midiSerial, *dev_id));
        break;
    case proto_Device_ws2812_tag:
        active_devices.emplace_back(new WS2812Device(device.device.ws2812, *dev_id));
        break;
    case proto_Device_stp16cpc_tag:
        active_devices.emplace_back(new STP16CPCDevice(device.device.stp16cpc, *dev_id));
        break;
    case proto_Device_apa102_tag:
        active_devices.emplace_back(new APA102Device(device.device.apa102, *dev_id));
        break;
    case proto_Device_multiplexer_tag:
        active_devices.emplace_back(new MultiplexerDevice(device.device.multiplexer, *dev_id));
        break;
    }
    *dev_id += 1;
    return true;
}
ShortcutInput *last_shortcut = nullptr;
std::unique_ptr<Input> make_input(proto_Input input, std::shared_ptr<Profile> profile, pb_istream_t *stream)
{
    switch (input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        return std::unique_ptr<Input>(new WiiAxisInput(input.input.wiiAxis, std::static_pointer_cast<WiiDevice>(profile->devices[input.input.wiiAxis.deviceid])));
    case proto_Input_wiiButton_tag:
        return std::unique_ptr<Input>(new WiiButtonInput(input.input.wiiButton, std::static_pointer_cast<WiiDevice>(profile->devices[input.input.wiiAxis.deviceid])));
    case proto_Input_crkd_tag:
        return std::unique_ptr<Input>(new CrkdButtonInput(input.input.crkd, std::static_pointer_cast<CrkdDevice>(profile->devices[input.input.crkd.deviceid])));
    case proto_Input_gpio_tag:
        return std::unique_ptr<Input>(new GPIOInput(input.input.gpio));
    case proto_Input_ads1115_tag:
        return std::unique_ptr<Input>(new ADS1115Input(input.input.ads1115, std::static_pointer_cast<ADS1115Device>(profile->devices[input.input.ads1115.deviceid])));
    case proto_Input_multiplexer_tag:
        return std::unique_ptr<Input>(new MultiplexerInput(input.input.multiplexer, std::static_pointer_cast<MultiplexerDevice>(profile->devices[input.input.multiplexer.deviceid])));
    case proto_Input_accelerometer_tag:
        return std::unique_ptr<Input>(new AccelerometerInput(input.input.accelerometer, std::static_pointer_cast<AccelerometerDevice>(profile->devices[input.input.accelerometer.deviceid])));
    case proto_Input_gh5Neck_tag:
        return std::unique_ptr<Input>(new Gh5ButtonInput(input.input.gh5Neck, std::static_pointer_cast<GH5NeckDevice>(profile->devices[input.input.gh5Neck.deviceid])));
    case proto_Input_fixed_tag:
        return std::unique_ptr<Input>(new FixedInput(input.input.fixed));
    case proto_Input_ps2Axis_tag:
        return std::unique_ptr<Input>(new PS2AxisInput(input.input.ps2Axis, std::static_pointer_cast<PS2Device>(profile->devices[input.input.ps2Axis.deviceid])));
    case proto_Input_ps2Button_tag:
        return std::unique_ptr<Input>(new PS2ButtonInput(input.input.ps2Button, std::static_pointer_cast<PS2Device>(profile->devices[input.input.ps2Button.deviceid])));
    case proto_Input_mpr121_tag:
        return std::unique_ptr<Input>(new MPR121Input(input.input.mpr121, std::static_pointer_cast<MPR121Device>(profile->devices[input.input.mpr121.deviceid])));
    case proto_Input_midiNote_tag:
        return std::unique_ptr<Input>(new MidiNoteInput(input.input.midiNote, std::static_pointer_cast<MidiDevice>(profile->devices[input.input.midiNote.deviceid])));
    case proto_Input_midiControlChange_tag:
        return std::unique_ptr<Input>(new MidiControlChangeInput(input.input.midiControlChange, std::static_pointer_cast<MidiDevice>(profile->devices[input.input.midiControlChange.deviceid])));
    case proto_Input_midiPitchBend_tag:
        return std::unique_ptr<Input>(new MidiPitchBendInput(input.input.midiPitchBend, std::static_pointer_cast<MidiDevice>(profile->devices[input.input.midiPitchBend.deviceid])));
    case proto_Input_mouseAxis_tag:
        return std::unique_ptr<Input>(new MouseAxisInput(input.input.mouseAxis, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.mouseAxis.deviceid])));
    case proto_Input_mouseButton_tag:
        return std::unique_ptr<Input>(new MouseButtonInput(input.input.mouseButton, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.mouseButton.deviceid])));
    case proto_Input_key_tag:
        return std::unique_ptr<Input>(new KeyboardKeyInput(input.input.key, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.key.deviceid])));
    case proto_Input_usbButton_tag:
        return std::unique_ptr<Input>(new USBButtonInput(input.input.usbButton, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.usbButton.deviceid])));
    case proto_Input_usbAxis_tag:
        return std::unique_ptr<Input>(new USBAxisInput(input.input.usbAxis, std::static_pointer_cast<UsbHostInterface>(profile->devices[input.input.usbAxis.deviceid])));
    case 0:
    {
        auto ret = last_shortcut;
        last_shortcut = nullptr;
        return std::unique_ptr<Input>(ret);
    }
    default:
        return nullptr;
    }
}
bool load_shortcut_input(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    proto_Input input;
    if (!pb_decode(stream, proto_Input_fields, &input))
    {
        printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    last_shortcut->inputs.push_back(make_input(input, profile, stream));
    printf("shortcut added: %d\r\n", last_shortcut->inputs.size());
    return true;
}
bool load_shortcut(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    printf("found shortcut!\r\n");
    last_shortcut = new ShortcutInput();
    proto_ShortcutInput input;
    input.inputs.arg = &profile;
    input.inputs.funcs.decode = &load_shortcut_input;
    if (!pb_decode(stream, proto_ShortcutInput_fields, &input))
    {
        printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    printf("loaded shortcut\r\n");
    return true;
}
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    proto_Mapping mapping;
    mapping.input.input.shortcut.arg = &profile;
    mapping.input.input.shortcut.funcs.decode = &load_shortcut;
    pb_decode(stream, proto_Mapping_fields, &mapping);
    std::unique_ptr<Input> input = make_input(mapping.input, profile, stream);
    if (input == nullptr)
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

bool load_assignment_info(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    auto &list = profile->triggers.back();
    proto_ProfileAssignmentInfo assignment;
    assignment.assignment.input.input.input.shortcut.arg = &profile;
    assignment.assignment.input.input.input.shortcut.funcs.decode = &load_shortcut;
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
        list->triggers.emplace_back(new InputActivationTrigger(false, assignment.assignment.input, std::move(input), profile->profile_id, list->triggers.size()));
        break;
    }
    case proto_ProfileAssignmentInfo_inputAnyTime_tag:
    {

        std::unique_ptr<Input> input = make_input(assignment.assignment.inputAnyTime.input, profile, stream);
        if (input == nullptr)
        {
            return true;
        }
        list->triggers.emplace_back(new InputActivationTrigger(true, assignment.assignment.inputAnyTime, std::move(input), profile->profile_id, list->triggers.size()));
        break;
    }
    case proto_ProfileAssignmentInfo_consoleType_tag:
        list->triggers.emplace_back(new ConsoleTypeActivationTrigger(assignment.assignment.consoleType, profile->profile_id));
        break;
    case proto_ProfileAssignmentInfo_wiiExt_tag:
        list->triggers.emplace_back(new WiiExtTypeActivationTrigger(assignment.assignment.wiiExt, profile->profile_id));
        break;
    case proto_ProfileAssignmentInfo_ps2Cnt_tag:
        list->triggers.emplace_back(new PS2ControllerTypeActivationTrigger(assignment.assignment.ps2Cnt, profile->profile_id));
        break;
    case proto_ProfileAssignmentInfo_usbType_tag:
        list->triggers.emplace_back(new UsbTypeActivationTrigger(assignment.assignment.usbType, profile->profile_id));
        break;
    case proto_ProfileAssignmentInfo_usbDevice_tag:
        list->triggers.emplace_back(new SpecificUsbDeviceActivationTrigger(assignment.assignment.usbDevice, profile->profile_id));
        break;
    case proto_ProfileAssignmentInfo_catchall_tag:
        if (assignment.assignment.catchall)
        {
            list->triggers.emplace_back(new CatchAllActivationTrigger(profile->profile_id));
        }
        break;
    case proto_ProfileAssignmentInfo_midiChannel_tag:
        list->triggers.emplace_back(new MidiChannelActivationTrigger(assignment.assignment.consoleType, profile->profile_id));
        break;
    case proto_ProfileAssignmentInfo_copilotProfile_tag:
        // TODO: how do we handle this
        break;
    }
    return true;
}

bool load_assignments(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    auto list = new ActivationTriggerList();
    profile->triggers.emplace_back(list);
    proto_ProfileAssignment proto_assignment;
    proto_assignment.assignments.funcs.decode = &load_assignment_info;
    proto_assignment.assignments.arg = &profile;
    pb_decode(stream, proto_ProfileAssignment_fields, &proto_assignment);
    list->validate(true, false, false);
    return true;
}
bool load_uid(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    uint64_t value;
    if (!pb_decode_varint(stream, &value))
        return false;
    profile->profile_id = value;
    all_profiles[profile->profile_id] = profile;
    return true;
}
bool load_leds(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = *(std::shared_ptr<Profile> *)*arg;
    std::unique_ptr<LedMappingDevice> device = nullptr;
    proto_Led proto_led;
    proto_led.mapping.led.inputMapping.input.input.shortcut.arg = &profile;
    proto_led.mapping.led.inputMapping.input.input.shortcut.funcs.decode = &load_shortcut;
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
    case proto_LedDevice_stp16_tag:
        device = std::make_unique<STP16CPCLedDevice>(proto_led.device.device.stp16, std::static_pointer_cast<STP16CPCDevice>(profile->devices[proto_led.device.device.stp16.deviceId]));
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
        printf("load device: %p %d\r\n", device.get(), device->m_id);
    }
    proto_Profile proto_profile;
    memset(&proto_profile, 0, sizeof(proto_profile));
    proto_profile.assignments.funcs.decode = &load_assignments;
    proto_profile.mappings.funcs.decode = &load_mapping;
    proto_profile.assignments.arg = &profile;
    proto_profile.mappings.arg = &profile;
    proto_profile.uid.arg = &profile;
    proto_profile.uid.funcs.decode = &load_uid;
    proto_profile.leds.arg = &profile;
    proto_profile.leds.funcs.decode = &load_leds;
    pb_decode(stream, proto_Profile_fields, &proto_profile);
    profile->subtype = proto_profile.deviceToEmulate;
    profile->xinput_on_windows = proto_profile.has_xinputOnWindows && proto_profile.xinputOnWindows;
    profile->invert_y_axis_hid = proto_profile.has_invertYAxisHid && proto_profile.invertYAxisHid;
    profile->supports_ps4 = proto_profile.has_ps4OrPs5Mode && proto_profile.ps4OrPs5Mode;
    printf("profile loaded: %d %d %d\r\n", profile->profile_id, profile->xinput_on_windows, profile->invert_y_axis_hid);
    // TODO: handle this once we support emulating non usb devices
    profile->output = OutputUSB;
    std::shared_ptr<UsbDevice> instance = nullptr;
    for (auto &list : profile->triggers)
    {
        if (list->validate(true, false, false))
        {
            printf("profile assigned! %d\r\n", profile->profile_id);
            if (instance == nullptr)
            {
                if (profile->output == OutputUSB)
                {
                    switch (mode)
                    {
                    case ModeHid:
                        instance = std::make_shared<HIDGamepadDevice>();
                        break;
                    case ModeOgXbox:
                        instance = std::make_shared<OGXboxGamepadDevice>();
                        break;
                    case ModeXbox360:
                        instance = std::make_shared<XInputGamepadDevice>();
                        break;
                    case ModeXboxOne:
                        instance = std::make_shared<XboxOneGamepadDevice>();
                        break;
                    case ModeWiiRb:
                        // wii rb is the same as ps3 but different ids
                        instance = std::make_shared<PS3GamepadDevice>(true);
                        break;
                    case ModePs3:
                        instance = std::make_shared<PS3GamepadDevice>(false);
                        break;
                    case ModePs4:
                        instance = std::make_shared<PS4GamepadDevice>();
                        break;
                    case ModePs5:
                        instance = std::make_shared<PS5GamepadDevice>();
                        break;
                    case ModeSwitch:
                        instance = std::make_shared<SwitchGamepadDevice>();
                        break;
                    case ModeGuitarHeroArcade:
                        instance = std::make_shared<GHArcadeGamepadDevice>();
                        break;
                    }
                    instances.push_back(instance);
                    instance->profiles.push_back(profile);
                    instance->interface_id = active_instances.size();
                    instance->subtype = profile->subtype;
                    instance->xinput_on_windows = profile->xinput_on_windows;
                    instance->invert_y_axis_hid = profile->invert_y_axis_hid;
                    instance->supports_ps4 = profile->supports_ps4;
                    active_instances.push_back(instance);
                    usb_instances[usb_instances.size()] = instance;
                    instance->initialize();
                }
                active_profiles.insert(profile->profile_id);
                printf("instance: %d\r\n", instance->interface_id);
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
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, 0);
    footer->magic = FOOTER_MAGIC;
    footer->currentProfile = 0;
    EEPROM.commit_now();
    return true;
}

bool inner_load(proto_Config &config, const uint32_t currentProfile, const uint8_t *dataPtr, uint32_t size)
{
    printf("inner_load\r\n");
    // We are now sufficiently confident that the data is valid so we run the deserialization
    // load just the current profile to begin with
    pb_istream_t inputStream = pb_istream_from_buffer(dataPtr, size);
    uint16_t dev_id = 0;
    config.devices.arg = &dev_id;
    config.devices.funcs.decode = &load_device;
    config.profiles.funcs.decode = &load_profile;
    assignable_devices.clear();
    instances.clear();
    active_instances.clear();
    usb_instances.clear();
    usb_instances_by_epnum.clear();
    active_devices.clear();
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
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 64);
    if (!pb_encode(&outputStream, proto_ConfigInfo_fields, &info))
    {
        return 0;
    }
    return outputStream.bytes_written;
}

void reload()
{
    reinit = true;
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
        printf("writing up to: %d < %d\r\n", start + bufsize, footer.dataSize);
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
    proto_Config config;
    EEPROM.commit();
    inner_load(config, footer.currentProfile, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, footer.dataSize);
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
bool load(proto_Config &config)
{
    config = proto_Config proto_Config_init_zero;

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

    return inner_load(config, footer.currentProfile, dataPtr, footer.dataSize);
}

void first_load()
{
    UsbDevice::reset_ep();
    active_devices.clear();
    instances.clear();
    active_instances.clear();
    usb_instances.clear();
    all_profiles.clear();
    auth_devices.clear();
    auto confDevice = std::make_shared<HIDConfigDevice>();
    confDevice->interface_id = instances.size();
    confDevice->initialize();
    instances.push_back(confDevice);
    active_instances.push_back(confDevice);
    usb_instances[confDevice->interface_id] = confDevice;
}