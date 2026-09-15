#pragma once
#include <stdint.h>
#include <string.h>
#include <memory>
#include "config.pb.h"
#include "enums.pb.h"
#include "devices/midi.hpp"
#include "utils.h"

// Forward declaration
class BluetoothHostInterface;

// Registry functions (implemented in bt_host.cpp)
size_t bt_host_assignable_interface_count();
void bt_host_add_assignable_devices(bool rescan);
void bt_host_add_interface(std::shared_ptr<BluetoothHostInterface> device);
void bt_host_add_assignable_interface(std::shared_ptr<BluetoothHostInterface> device);
void bt_host_promote_if_ready(std::shared_ptr<BluetoothHostInterface> device);
void bt_host_remove_interface(BluetoothHostInterface *device);
void bt_host_remove_assignable_interface(BluetoothHostInterface *device);
void bt_host_update_interfaces(bool full_poll, bool send_events);
void bt_host_notify_devices_changed();
void bt_host_save_pairing(const std::shared_ptr<BluetoothHostInterface> &device, bool is_ble);

// Avoid pulling btstack.h into the header (it conflicts with TinyUSB HID enum).
// The BTstack-facing fields use plain types; each .cpp that calls BTstack APIs
// includes btstack.h itself before including this header.
typedef uint8_t bt_addr_t[6];
typedef uint8_t bt_addr_type_t;

/**
 * BluetoothHostInterface — base class for all BT RX host implementations.
 *
 * Transport (BT Classic HID or BLE HIDS) provides raw HID report bytes via
 * handle_report().  Subclasses implement tick_digital() / tick_analog() by
 * casting the internal buffer to the appropriate protocol struct, exactly as
 * the USB host classes do.
 */
class BluetoothHostInterface : public MidiDevice
{
public:
    virtual ~BluetoothHostInterface() { printf("~BluetoothHostInterface()\r\n"); }

    BluetoothHostInterface(uint16_t id)
        : MidiDevice(nullptr, id, true, m_midi_buffers.config())
    {
        memset(m_name, 0, sizeof(m_name));
        memset(m_report_buf, 0, sizeof(m_report_buf));
    }

    // Called by btstack when an HID report arrives
    virtual void handle_report(const uint8_t *data, uint16_t len)
    {
        uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
        memcpy(m_report_buf, data, copy_len);
    }

    // Called when a feature report response arrives (e.g. PS4/PS5 capabilities)
    virtual void handle_feature_report(const uint8_t *data, uint16_t len) {}
    virtual void handle_feature_report_failed() { m_ready = true; }

    // Readiness: whether this host can be registered as an assignable device immediately.
    // 3rd-party PS4/PS5 controllers hold off until feature report 0x03 provides their true subtype.
    virtual bool is_ready() const { return m_ready; }
    void set_ready(bool ready = true) { m_ready = ready; }
    bool is_registered() const { return m_registered; }
    void set_registered(bool reg = true) { m_registered = reg; }
    uint32_t connected_at() const { return m_connected_at; }

    // Called once connection info is ready
    virtual void on_connected()
    {
        m_connected_at = millis();
    }

    // Emits initial device connected event or updated info
    void notify_connected();
    void notify_subtype_changed() { notify_connected(); }

    // Called when the connection drops (fires the device-removed event)
    virtual void on_disconnected();

    // Tick interface — same as UsbHostInterface
    virtual bool tick_digital(proto_Output &type) = 0;
    virtual uint16_t tick_analog(proto_Output &type) = 0;

    // Device lifecycle (mirrors UsbHostInterface)
    virtual void update(bool full_poll, bool send_events);
    void begin() {}
    void end(bool full) {}
    bool is_assignable() const override { return true; }
    bool using_pin(uint8_t pin) { return false; }
    bool is_usb_type(SubType type) { return false; }
    bool is_bluetooth_type(SubType type) { return type == m_subtype; }
    bool is_bluetooth_device(proto_SpecificBluetoothDevice type) { return false; }
    bool is_usb_device(proto_SpecificUsbDevice type) { return false; }
    bool is_wii_extension(WiiExtType type) { return false; }
    bool is_ps2_device(PS2ControllerType type) { return false; }

    SubType subtype() const { return m_subtype; }
    virtual BtControllerType controller_type() const { return BtControllerType_BtControllerTypeGeneric; }
    uint16_t vid() const { return m_vid; }
    uint16_t pid() const { return m_pid; }
    bool is_ble() const { return m_is_ble; }
    void set_ble(bool ble = true) { m_is_ble = ble; }

    // Human-readable source ID for events  (id in high 16 bits, addr in low 16)
    uint32_t source_id() const override
    {
        // Use the lower 4 bytes of the BT address as a unique identifier
        return (static_cast<uint32_t>(m_id) << 16) |
               (static_cast<uint32_t>(m_addr[4]) << 8) |
               static_cast<uint32_t>(m_addr[5]);
    }

    // BT address + transport info set by the Classic/BLE rx code before calling on_connected()
    bt_addr_t m_addr = {};
    bt_addr_type_t m_addr_type = 0; // 0 = public
    uint16_t m_cid = 0;        // BT Classic: hid_host_cid; BLE: hids_cid
    uint16_t m_vid = 0;
    uint16_t m_pid = 0;

    // These are public so bt_classic_rx.cpp/ble_rx.cpp can populate them after creation
    SubType m_subtype = SubType_Gamepad;
    char m_name[100] = {};
    bool m_is_ble = false;

protected:
    bool m_ready = true;
    bool m_registered = false;
    uint32_t m_connected_at = 0;
    bool m_sent_type = false;
    uint8_t m_report_buf[128] = {};  // large enough for any HID report we handle

    MidiStaticBuffers<64, 0, 64, 1> m_midi_buffers;
};
