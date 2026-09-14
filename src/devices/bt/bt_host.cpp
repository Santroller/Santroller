#include "devices/bt/bt_host.hpp"
#include "managers/device_manager.hpp"
#include "emulation/usb/hid_device.h"
#include "events.pb.h"
#include "config/config.hpp"
#include "config/device_factory.hpp"
#include "utils.h"

#include <vector>
#include <algorithm>
#include <memory>

// ------------------------------------------------------------------
// Global lists of BT host interfaces
// ------------------------------------------------------------------
static std::vector<std::shared_ptr<BluetoothHostInterface>> bt_pending_interfaces;
static std::vector<std::shared_ptr<BluetoothHostInterface>> bt_assignable_interfaces;

void bt_host_add_assignable_interface(std::shared_ptr<BluetoothHostInterface> device)
{
    bt_host_add_interface(device);
}

void bt_host_add_interface(std::shared_ptr<BluetoothHostInterface> device)
{
    if (device->is_ready())
    {
        if (!device->is_registered())
        {
            device->set_registered(true);
            bt_assignable_interfaces.push_back(device);
            DeviceManager::instance().add_assignable_device(device);
            device->notify_connected();
        }
    }
    else
    {
        // Hold off on registering as assignable until subtype is fetched
        bt_pending_interfaces.push_back(device);
    }
}

void bt_host_promote_if_ready(std::shared_ptr<BluetoothHostInterface> device)
{
    if (!device || !device->is_ready() || device->is_registered())
        return;

    bt_pending_interfaces.erase(
        std::remove_if(bt_pending_interfaces.begin(), bt_pending_interfaces.end(),
                       [&device](const auto &d) { return d.get() == device.get(); }),
        bt_pending_interfaces.end());

    device->set_registered(true);
    bt_assignable_interfaces.push_back(device);
    DeviceManager::instance().add_assignable_device(device);
    device->notify_connected();
}

void bt_host_remove_assignable_interface(BluetoothHostInterface *device)
{
    bt_host_remove_interface(device);
}

void bt_host_remove_interface(BluetoothHostInterface *device)
{
    bt_pending_interfaces.erase(
        std::remove_if(bt_pending_interfaces.begin(), bt_pending_interfaces.end(),
                       [device](const auto &d) { return d.get() == device; }),
        bt_pending_interfaces.end());

    bt_assignable_interfaces.erase(
        std::remove_if(bt_assignable_interfaces.begin(), bt_assignable_interfaces.end(),
                       [device](const auto &d) { return d.get() == device; }),
        bt_assignable_interfaces.end());

    DeviceManager::instance().remove_device(device);
}

void bt_host_update_interfaces(bool full_poll, bool send_events)
{
    // Check pending interfaces for readiness or timeout
    auto it = bt_pending_interfaces.begin();
    while (it != bt_pending_interfaces.end())
    {
        auto device = *it;
        if (device->is_ready())
        {
            it = bt_pending_interfaces.erase(it);
            device->set_registered(true);
            bt_assignable_interfaces.push_back(device);
            DeviceManager::instance().add_assignable_device(device);
            device->notify_connected();
        }
        else if (millis() - device->connected_at() > 2000)
        {
            printf("BT device %lu timed out waiting for capabilities, defaulting to subtype=%d\r\n",
                   (unsigned long)device->source_id(), (int)device->subtype());
            device->handle_feature_report_failed();
            it = bt_pending_interfaces.erase(it);
            device->set_registered(true);
            bt_assignable_interfaces.push_back(device);
            DeviceManager::instance().add_assignable_device(device);
            device->notify_connected();
        }
        else
        {
            ++it;
        }
    }

    for (const auto &device : bt_assignable_interfaces)
    {
        device->update(full_poll, send_events);
    }
}

void bt_host_notify_devices_changed()
{
}

void bt_host_save_pairing(const std::shared_ptr<BluetoothHostInterface> &device, bool is_ble)
{
    if (!device)
        return;

    int32_t id = DeviceFactory::find_bluetooth_pairing_id_by_mac(device->m_addr);
    if (id < 0)
    {
        id = DeviceFactory::allocate_bluetooth_pairing_id();
    }
    DeviceFactory::BluetoothPairingStateData existing;
    const uint8_t *link_key = nullptr;
    if (DeviceFactory::find_bluetooth_pairing_state_by_mac(device->m_addr, existing) && existing.has_link_key)
    {
        link_key = existing.link_key;
    }
    update_aux_bluetooth_pairing(id, device->m_addr, device->m_name, is_ble,
                                 device->subtype(), device->controller_type(),
                                 device->vid(), device->pid(), link_key);
    printf("BT pairing saved: id=%ld, MAC=%02x:%02x:%02x:%02x:%02x:%02x, name='%s', ble=%d, subtype=%d, type=%d, vid=0x%04x, pid=0x%04x\r\n",
           (long)id, device->m_addr[0], device->m_addr[1], device->m_addr[2],
           device->m_addr[3], device->m_addr[4], device->m_addr[5],
           device->m_name, (int)is_ble, (int)device->subtype(), (int)device->controller_type(),
           device->vid(), device->pid());
}

// ------------------------------------------------------------------
// BluetoothHostInterface member implementations
// ------------------------------------------------------------------

void BluetoothHostInterface::on_disconnected()
{
    still_connected = false;
    if (m_registered)
    {
        proto_Event event = proto_Event_init_default;
        event.which_event = proto_Event_bt_tag;
        event.event.bt.id = m_id;
        event.event.bt.subtype = m_subtype;
        event.event.bt.connected = false;
        event.event.bt.has_sourceId = true;
        event.event.bt.sourceId = source_id();
        HIDConfigDevice::send_event(event, true);
    }

    bt_host_remove_interface(this);
}

void BluetoothHostInterface::update(bool full_poll, bool send_events)
{
    still_connected = true;
    MidiDevice::update(full_poll, send_events);
    if (send_events && full_poll)
    {
        notify_connected();
    }
}

void BluetoothHostInterface::notify_connected()
{
    m_sent_type = true;
    proto_Event event = proto_Event_init_default;
    event.which_event = proto_Event_bt_tag;
    event.event.bt.id = m_id;
    event.event.bt.subtype = m_subtype;
    event.event.bt.connected = true;
    event.event.bt.has_sourceId = true;
    event.event.bt.sourceId = source_id();

    size_t max_len = sizeof(event.event.bt.name) - 1;
    strncpy(event.event.bt.name, m_name, max_len);
    event.event.bt.name[max_len] = '\0';

    HIDConfigDevice::send_event(event, true);
}
