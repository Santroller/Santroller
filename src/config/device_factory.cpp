#include "config/device_factory.hpp"
#include "devices/accelerometer.hpp"
#include "devices/crkd.hpp"
#include "devices/crkd_drum.hpp"
#include "devices/wii.hpp"
#include "devices/ps2.hpp"
#include "devices/ps2_emulation.hpp"
#include "devices/wii_emulation.hpp"
#include "devices/protar_neck.hpp"
#include "devices/bhdrum.hpp"
#include "devices/wtdrum.hpp"
#include "devices/crazyneck.hpp"
#include "devices/djh.hpp"
#include "devices/gh5neck.hpp"
#include "devices/max1704x.hpp"
#include "devices/mpr121.hpp"
#include "devices/usb.hpp"
#include "devices/ads1115.hpp"
#include "devices/debug.hpp"
#include "devices/midiserial.hpp"
#include "devices/ws2812.hpp"
#include "devices/stp16cpc.hpp"
#include "devices/apa102.hpp"
#include "devices/multiplexer.hpp"
#include "devices/bluetooth.hpp"
#include "devices/vtechexpander.hpp"
#include "devices/matrix.hpp"
#include "devices/cycle.hpp"
#include "devices/toggle.hpp"
#include "devices/dmx.hpp"

// Static storage for emulation devices and state
static std::map<int32_t, int32_t> s_cycle_states;
static std::map<int32_t, bool> s_toggle_states;
static std::vector<uint32_t> s_last_cycle_states;

// Cycle state management
void DeviceFactory::set_cycle_state(int32_t id, int32_t state) {
    s_cycle_states[id] = state;
}

int32_t DeviceFactory::get_cycle_state(int32_t id) {
    auto it = s_cycle_states.find(id);
    return it != s_cycle_states.end() ? it->second : 0;
}

void DeviceFactory::add_last_cycle_state(uint32_t state) {
    s_last_cycle_states.push_back(state);
}

void DeviceFactory::clear_cycle_states() {
    s_cycle_states.clear();
    s_last_cycle_states.clear();
}

void DeviceFactory::foreach_cycle_state(std::function<void(int32_t id, int32_t state)> callback) {
    for (auto& state : s_cycle_states) {
        callback(state.first, state.second);
    }
}

// Toggle state management
void DeviceFactory::set_toggle_state(int32_t id, bool state) {
    s_toggle_states[id] = state;
}

bool DeviceFactory::get_toggle_state(int32_t id) {
    auto it = s_toggle_states.find(id);
    return it != s_toggle_states.end() ? it->second : false;
}

void DeviceFactory::clear_toggle_states() {
    s_toggle_states.clear();
}

void DeviceFactory::foreach_toggle_state(std::function<void(int32_t id, bool state)> callback) {
    for (auto& state : s_toggle_states) {
        callback(state.first, state.second);
    }
}

const std::vector<uint32_t>& DeviceFactory::get_last_cycle_states() {
    return s_last_cycle_states;
}

std::shared_ptr<Device> DeviceFactory::create_device(
    const proto_Device& proto_device,
    uint32_t device_id,
    const DeviceReloadState* previous_state)
{
    std::shared_ptr<Device> device;
    
    switch (proto_device.which_device)
    {
    case proto_Device_accelerometer_tag:
        device = std::make_shared<AccelerometerDevice>(proto_device.device.accelerometer, device_id);
        break;
        
    case proto_Device_crkdNeck_tag:
        device = std::make_shared<CrkdDevice>(proto_device.device.crkdNeck, device_id);
        break;
        
    case proto_Device_crkdDrum_tag:
        device = std::make_shared<CrkdDrumDevice>(proto_device.device.crkdDrum, device_id);
        break;
        
    case proto_Device_wii_tag:
        // Preserve state from previous device
        device = std::make_shared<WiiDevice>(
            previous_state,
            proto_device.device.wii,
            device_id
        );
        break;
        
    case proto_Device_psx_tag:
        // Preserve state from previous device
        device = std::make_shared<PS2Device>(
            previous_state,
            proto_device.device.psx,
            device_id
        );
        break;
        
    case proto_Device_psxEmulation_tag:
        device = std::make_shared<PSXEmulationDevice>(proto_device.device.psxEmulation, device_id);
        break;
        
    case proto_Device_wiiEmulation_tag:
        device = std::make_shared<WiiEmulationDevice>(proto_device.device.wiiEmulation, device_id);
        break;
        
    case proto_Device_protarNeck_tag:
        device = std::make_shared<ProtarNeckDevice>(proto_device.device.protarNeck, device_id);
        break;
        
    case proto_Device_bhDrum_tag:
        device = std::make_shared<BandHeroDrumDevice>(
            previous_state,
            proto_device.device.bhDrum,
            device_id
        );
        break;
        
    case proto_Device_worldTourDrum_tag:
        device = std::make_shared<WorldTourDrumDevice>(
            previous_state,
            proto_device.device.worldTourDrum,
            device_id
        );
        break;
        
    case proto_Device_crazyGuitarNeck_tag:
        device = std::make_shared<CrazyGuitarNeckDevice>(proto_device.device.crazyGuitarNeck, device_id);
        break;
        
    case proto_Device_djhTurntable_tag:
        device = std::make_shared<DjHeroTurntableDevice>(proto_device.device.djhTurntable, device_id);
        break;
        
    case proto_Device_gh5Neck_tag:
        device = std::make_shared<GH5NeckDevice>(proto_device.device.gh5Neck, device_id);
        break;
        
    case proto_Device_max1704x_tag:
        device = std::make_shared<Max1704XDevice>(proto_device.device.max1704x, device_id);
        break;
        
    case proto_Device_mpr121_tag:
        device = std::make_shared<MPR121Device>(proto_device.device.mpr121, device_id);
        break;
        
    case proto_Device_usbHost_tag:
        device = std::make_shared<USBHostHardwareDevice>(proto_device.device.usbHost, device_id);
        break;
        
    case proto_Device_ads1115_tag:
        device = std::make_shared<ADS1115Device>(proto_device.device.ads1115, device_id);
        break;
        
    case proto_Device_debug_tag:
        device = std::make_shared<DebugDevice>(proto_device.device.debug, device_id);
        break;
        
    case proto_Device_midiSerial_tag:
        device = std::make_shared<MidiSerialDevice>(
            previous_state,
            proto_device.device.midiSerial,
            device_id
        );
        break;
        
    case proto_Device_ws2812_tag:
        device = std::make_shared<WS2812Device>(proto_device.device.ws2812, device_id);
        break;
        
    case proto_Device_stp16cpc_tag:
        device = std::make_shared<STP16CPCDevice>(proto_device.device.stp16cpc, device_id);
        break;
        
    case proto_Device_apa102_tag:
        device = std::make_shared<APA102Device>(proto_device.device.apa102, device_id);
        break;
        
    case proto_Device_multiplexer_tag:
        device = std::make_shared<MultiplexerDevice>(proto_device.device.multiplexer, device_id);
        break;
        
    case proto_Device_bt_tag:
        device = std::make_shared<BluetoothDevice>(proto_device.device.bt, device_id);
        break;
        
    case proto_Device_vtechExpander_tag:
        device = std::make_shared<VTechGuitarIOExpanderDevice>(proto_device.device.vtechExpander, device_id);
        break;
        
    case proto_Device_matrix_tag:
        device = std::make_shared<MatrixDevice>(proto_device.device.matrix, device_id);
        break;
        
    case proto_Device_cycle_tag:
        device = std::make_shared<CycleDevice>(
            previous_state,
            proto_device.device.cycle,
            device_id,
            s_cycle_states[device_id],
            s_last_cycle_states
        );
        s_last_cycle_states.clear();
        break;
        
    case proto_Device_toggle_tag:
        device = std::make_shared<ToggleDevice>(
            previous_state,
            proto_device.device.toggle,
            device_id,
            s_toggle_states[device_id]
        );
        break;
        
    case proto_Device_dmx_tag:
        device = std::make_shared<DMXDevice>(proto_device.device.dmx, device_id);
        break;
    }
    
    return device;
}
