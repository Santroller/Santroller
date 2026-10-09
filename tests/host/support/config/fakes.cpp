// Link time fakes for the config tests: NOR flash, the state half of DeviceFactory, and stand-ins for
// config.cpp's load_device / load_profile (which build real devices and profiles) that just record
// what they were handed
#include <cassert>
#include <cstring>
#include <mutex>
#include <thread>
#include <pb_decode.h>
#include "config/FlashPROM.h"
#include "config/device_factory.hpp"
#include "config/profile_opts.hpp"
#include "devices/bt/bt_tlv_storage.hpp"
#include "config_fakes.hpp"
#include "config_test_support.hpp"

// NOR flash: erase sets whole sectors to 0xFF, programming can only clear bits
void flash_range_erase(uint32_t flash_offs, size_t count)
{
    assert(flash_offs % FLASH_SECTOR_SIZE == 0);
    assert(count % FLASH_SECTOR_SIZE == 0);
    assert(flash_offs + count <= PICO_FLASH_SIZE_BYTES);
    memset(fake_flash::memory + flash_offs, 0xFF, count);
    fake_flash::erase_count++;
}

void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count)
{
    assert(flash_offs % FLASH_PAGE_SIZE == 0);
    assert(count % FLASH_PAGE_SIZE == 0);
    assert(flash_offs + count <= PICO_FLASH_SIZE_BYTES);
    for (size_t i = 0; i < count; i++)
    {
        uint8_t &cell = fake_flash::memory[flash_offs + i];
        if ((cell & data[i]) != data[i])
        {
            fake_flash::program_without_erase = true;
        }
        cell &= data[i];
    }
    fake_flash::program_count++;
}

namespace config_test
{
void ensure_core1_running()
{
    // flash_core1_loop never returns, like core1 on the Pico, so it gets its own thread for the process
    static std::once_flag started;
    std::call_once(started, []
                   { std::thread(flash_core1_loop).detach(); });
}
}

// Same as the state half of src/config/device_factory.cpp, which can't be built here without every device
static std::map<int32_t, int32_t> s_cycle_states;
static std::map<int32_t, bool> s_toggle_states;
static std::map<int32_t, DeviceFactory::BluetoothPairingStateData> s_bluetooth_pairing_states;
static std::vector<uint32_t> s_last_cycle_states;

void DeviceFactory::set_cycle_state(int32_t id, int32_t state) { s_cycle_states[id] = state; }

int32_t DeviceFactory::get_cycle_state(int32_t id)
{
    auto it = s_cycle_states.find(id);
    return it != s_cycle_states.end() ? it->second : 0;
}

void DeviceFactory::add_last_cycle_state(uint32_t state) { s_last_cycle_states.push_back(state); }

void DeviceFactory::clear_cycle_states()
{
    s_cycle_states.clear();
    s_last_cycle_states.clear();
}

void DeviceFactory::foreach_cycle_state(std::function<void(int32_t id, int32_t state)> callback)
{
    for (auto &state : s_cycle_states)
    {
        callback(state.first, state.second);
    }
}

void DeviceFactory::set_toggle_state(int32_t id, bool state) { s_toggle_states[id] = state; }

bool DeviceFactory::get_toggle_state(int32_t id)
{
    auto it = s_toggle_states.find(id);
    return it != s_toggle_states.end() ? it->second : false;
}

void DeviceFactory::clear_toggle_states() { s_toggle_states.clear(); }

static uint8_t s_arcade_side = 1;
void DeviceFactory::set_arcade_side(uint8_t side) { s_arcade_side = side; }
uint8_t DeviceFactory::get_arcade_side() { return s_arcade_side; }
void DeviceFactory::clear_arcade_side() { s_arcade_side = 1; }

void DeviceFactory::foreach_toggle_state(std::function<void(int32_t id, bool state)> callback)
{
    for (auto &state : s_toggle_states)
    {
        callback(state.first, state.second);
    }
}

void DeviceFactory::set_bluetooth_pairing_state(int32_t id, const uint8_t mac[6], const char *name, bool ble,
                                                SubType subtype, BtControllerType controller_type,
                                                uint16_t vid, uint16_t pid,
                                                const uint8_t *link_key)
{
    BluetoothPairingStateData &state = s_bluetooth_pairing_states[id];
    memcpy(state.mac, mac, sizeof(state.mac));
    if (name)
    {
        strncpy(state.name, name, sizeof(state.name) - 1);
        state.name[sizeof(state.name) - 1] = '\0';
    }
    else
    {
        state.name[0] = '\0';
    }
    state.ble = ble;
    state.subtype = subtype;
    state.controller_type = controller_type;
    state.vid = vid;
    state.pid = pid;
    if (link_key)
    {
        state.has_link_key = true;
        memcpy(state.link_key, link_key, sizeof(state.link_key));
    }
    else if (!state.has_link_key)
    {
        state.has_link_key = false;
        memset(state.link_key, 0, sizeof(state.link_key));
    }
}

void DeviceFactory::set_bluetooth_pairing_link_key(int32_t id, const uint8_t key[16])
{
    auto it = s_bluetooth_pairing_states.find(id);
    if (it != s_bluetooth_pairing_states.end())
    {
        it->second.has_link_key = key != nullptr;
        if (key)
        {
            memcpy(it->second.link_key, key, 16);
        }
        else
        {
            memset(it->second.link_key, 0, sizeof(it->second.link_key));
        }
    }
}

bool DeviceFactory::get_bluetooth_pairing_state(int32_t id, BluetoothPairingStateData &out)
{
    auto it = s_bluetooth_pairing_states.find(id);
    if (it == s_bluetooth_pairing_states.end())
    {
        return false;
    }
    out = it->second;
    return true;
}

int32_t DeviceFactory::find_bluetooth_pairing_id_by_mac(const uint8_t mac[6])
{
    for (const auto &pair : s_bluetooth_pairing_states)
    {
        if (memcmp(pair.second.mac, mac, 6) == 0)
        {
            return pair.first;
        }
    }
    return -1;
}

void DeviceFactory::clear_bluetooth_pairing_states()
{
    s_bluetooth_pairing_states.clear();
    BtTlvStorage::instance().clear();
}

void DeviceFactory::foreach_bluetooth_pairing_state(std::function<void(int32_t id, const BluetoothPairingStateData &state)> callback)
{
    for (auto &state : s_bluetooth_pairing_states)
    {
        callback(state.first, state.second);
    }
}

const std::vector<uint32_t> &DeviceFactory::get_last_cycle_states() { return s_last_cycle_states; }

// Stand-ins for config.cpp's callbacks. They decode the same way (default-initialised messages decoded
// with PB_DECODE_NOINIT so the callbacks set beforehand survive), but only record the result.
static bool fake_cycle_value(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    uint64_t value;
    if (!pb_decode_varint(stream, &value))
    {
        return false;
    }
    static_cast<std::vector<int32_t> *>(*arg)->push_back(static_cast<int32_t>(value));
    return true;
}

static bool fake_device_dev(pb_istream_t *, const pb_field_t *field, void **arg)
{
    if (field->tag == proto_Device_cycle_tag)
    {
        auto *msg = static_cast<proto_CycleDevice *>(field->pData);
        msg->values.funcs.decode = fake_cycle_value;
        msg->values.arg = *arg;
    }
    return true;
}

bool load_device(pb_istream_t *stream, const pb_field_t *, void **)
{
    fake_loader::LoadedDevice loaded;
    loaded.device = proto_Device_init_default;
    loaded.device.cb_device.funcs.decode = fake_device_dev;
    loaded.device.cb_device.arg = &loaded.cycle_values;
    if (!pb_decode_ex(stream, proto_Device_fields, &loaded.device, PB_DECODE_NOINIT))
    {
        return false;
    }
    loaded.device.cb_device = {};
    fake_loader::state.calls.push_back("load_device");
    fake_loader::state.devices.push_back(std::move(loaded));
    return true;
}

static bool fake_opts(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    auto *profile = static_cast<fake_loader::LoadedProfile *>(*arg);
    profile->opts = proto_ProfileOpts_init_default;
    profile->has_opts = decode_profile_opts(stream, &profile->opts);
    return profile->has_opts;
}

static bool fake_mapping(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    auto *profile = static_cast<fake_loader::LoadedProfile *>(*arg);
    proto_Mapping mapping = proto_Mapping_init_default;
    if (!pb_decode_ex(stream, proto_Mapping_fields, &mapping, PB_DECODE_NOINIT))
    {
        return false;
    }
    profile->mappings.push_back(mapping);
    return true;
}

static bool fake_led(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    auto *profile = static_cast<fake_loader::LoadedProfile *>(*arg);
    proto_Led led = proto_Led_init_default;
    if (!pb_decode_ex(stream, proto_Led_fields, &led, PB_DECODE_NOINIT))
    {
        return false;
    }
    profile->leds.push_back(led);
    return true;
}

static bool fake_assignment_info(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    auto *profile = static_cast<fake_loader::LoadedProfile *>(*arg);
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_default;
    if (!pb_decode_ex(stream, proto_ProfileAssignmentInfo_fields, &info, PB_DECODE_NOINIT))
    {
        return false;
    }
    profile->assignments.push_back(info);
    return true;
}

static bool fake_assignments(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    auto *profile = static_cast<fake_loader::LoadedProfile *>(*arg);
    proto_ProfileAssignment assignment = proto_ProfileAssignment_init_default;
    assignment.assignments.funcs.decode = fake_assignment_info;
    assignment.assignments.arg = profile;
    profile->assignment_lists++;
    return pb_decode_ex(stream, proto_ProfileAssignment_fields, &assignment, PB_DECODE_NOINIT);
}

bool load_profile(pb_istream_t *stream, const pb_field_t *, void **)
{
    fake_loader::LoadedProfile loaded;
    proto_Profile profile = proto_Profile_init_default;
    profile.opts.funcs.decode = fake_opts;
    profile.opts.arg = &loaded;
    profile.mappings.funcs.decode = fake_mapping;
    profile.mappings.arg = &loaded;
    profile.leds.funcs.decode = fake_led;
    profile.leds.arg = &loaded;
    profile.assignments.funcs.decode = fake_assignments;
    profile.assignments.arg = &loaded;
    if (!pb_decode_ex(stream, proto_Profile_fields, &profile, PB_DECODE_NOINIT))
    {
        return false;
    }
    fake_loader::state.calls.push_back("load_profile");
    fake_loader::state.profiles.push_back(std::move(loaded));
    return true;
}
