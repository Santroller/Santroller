#include "devices/usb/host/xbox_wireless_controller.h"
#include "devices/usb/host/xbox_wireless_host.h"
#include "usb/auth_broker.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb.hpp"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"

extern "C" {
#include "gip_packet_handler.h"
#include "gip_device.h"
#include "gip_button_mapping.h"
}

#include "../../include/protocols/xbox_one.hpp"

#include <stdio.h>

static void wireless_on_device_descriptor_wrapper(void *context, SubType subtype);
static void wireless_on_arrival_wrapper(void *context);
static void wireless_queue_packet_wrapper(void *context, const uint8_t *data, uint16_t len);
static void wireless_send_ack_wrapper(void *context, const uint8_t *data, uint16_t len);
static void wireless_on_disconnect_wrapper(void *context);
static uint16_t wireless_process_secondary_wrapper(void *context, const uint8_t *data, uint16_t len);

static const gip_device_interface_t wireless_gip_interface = {
    .on_device_descriptor = wireless_on_device_descriptor_wrapper,
    .on_arrival = wireless_on_arrival_wrapper,
    .queue_packet = wireless_queue_packet_wrapper,
    .send_ack = wireless_send_ack_wrapper,
    .on_disconnect = wireless_on_disconnect_wrapper,
    .process_secondary = wireless_process_secondary_wrapper
};

XboxWirelessController::XboxWirelessController(XboxWirelessHost* adapter, uint8_t controller_idx, uint8_t dev_addr, uint16_t id)
    // Virtual interface number, kept clear of the adapter's real interfaces so the config
    // tool doesn't confuse controller 0 with the adapter itself
    : UsbHostInterface(dev_addr, XBOX_WIRELESS_CONTROLLER_INTERFACE(controller_idx), id)
    , m_controller_idx(controller_idx)
    , m_adapter(adapter)
{
    m_delayed_init = false;
    
    memset(&m_controller, 0, sizeof(m_controller));
    m_controller.device_type = XBOX_DEVICE_UNKNOWN;
    gip_device_init(&m_controller.gip_device);
    m_controller.gip_device.user_context = this;
    m_controller.gip_device.interface = &wireless_gip_interface;
    m_controller.status = XBOX_CONTROLLER_DISCONNECTED;
    
    printf("XboxWirelessController: Created virtual interface for controller %d (%p)\r\n", controller_idx, this);
}

XboxWirelessController::~XboxWirelessController()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModeXboxOne);
        m_auth_registered = false;
    }
    gip_device_cleanup(&m_controller.gip_device);
}

void XboxWirelessController::disconnect()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModeXboxOne);
        m_auth_registered = false;
    }

    UsbHostInterface::disconnect();
}

void XboxWirelessController::send_report_from_host(XGIPProtocol *report)
{
    // The console is now driving auth, so any earlier local auth-complete is stale.
    // The forwarded packet itself may be the console's auth-complete.
    m_controller.gip_device.auth_complete_sent = false;
    m_controller.gip_device.outgoing_xgip->copyAttributes(report);
    if (m_controller.gip_device.outgoing_xgip->getSequence() == 0) {
        uint8_t seq = gip_sequence_pool_next(&m_controller.gip_device.tx_sequence_pools, m_controller.gip_device.outgoing_xgip->getCommand());
        m_controller.gip_device.outgoing_xgip->setSequence(seq);
    }
    uint8_t *packet = m_controller.gip_device.outgoing_xgip->generatePacket();
    m_adapter->send_report_from_host(m_controller_idx + 1, m_controller.mac_addr, packet,
                                     m_controller.gip_device.outgoing_xgip->getPacketLength());
    // The rest of a fragmented message waits for the controller to acknowledge this one
    if (m_controller.gip_device.outgoing_xgip->getPacketAck()) {
        gip_device_set_ack_wait(&m_controller.gip_device, to_ms_since_boot(get_absolute_time()));
    }
}
bool XboxWirelessController::tick_digital(proto_Output &type)
{
    xbox_controller_t* controller = get_controller_data();
    
    if (!controller || controller->status != XBOX_CONTROLLER_READY)
    {
        return false;
    }
    
    return gip_tick_digital(controller->gip_device.raw_input, m_subtype, controller->gip_device.capture, &type);
}

uint16_t XboxWirelessController::tick_analog(proto_Output &type)
{
    xbox_controller_t* controller = get_controller_data();
    
    if (!controller || controller->status != XBOX_CONTROLLER_READY)
    {
        return 0;
    }
    
    return gip_tick_analog(controller->gip_device.raw_input, m_subtype, &type);
}

void XboxWirelessController::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    
    xbox_controller_t* controller = get_controller_data();
    if (!controller || controller->status == XBOX_CONTROLLER_DISCONNECTED)
    {
        return;
    }
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    auto queue_lambda = [](void *context, const uint8_t *data, uint16_t len) {
        auto controller = static_cast<XboxWirelessController *>(context);
        controller->m_adapter->send_report_from_host(controller->m_controller_idx + 1,
                                                     controller->m_controller.mac_addr, data, len);
    };
    gip_device_update(&controller->gip_device, now, XGIP_ACK_WAIT_TIMEOUT, queue_lambda, this);
    if (m_chatpad)
    {
        m_chatpad->check_presence();
    }
}

uint16_t XboxWirelessController::process_secondary(const uint8_t *data, uint16_t len)
{
    if (!m_chatpad)
    {
        m_chatpad = std::make_shared<XboxOneChatpadHost>(m_dev_addr, XBOX_WIRELESS_CHATPAD_INTERFACE(m_controller_idx), m_id, m_adapter->get_controller_interface(m_controller_idx), [](UsbHostInterface &controller, const uint8_t *data, uint16_t len, bool ack)
                                                         {
                                                             auto &wireless = static_cast<XboxWirelessController &>(controller);
                                                             if (ack)
                                                             {
                                                                 wireless.send_ack_packet(data, len);
                                                             }
                                                             else
                                                             {
                                                                 wireless.send_gip_packet(data, len);
                                                             } });
    }
    return m_chatpad->process(data, len);
}

void XboxWirelessController::remove_chatpad()
{
    if (m_chatpad)
    {
        m_chatpad->lost();
        DeviceManager::instance().remove_device(m_chatpad.get());
        m_chatpad.reset();
    }
}

static uint16_t wireless_process_secondary_wrapper(void *context, const uint8_t *data, uint16_t len)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    return ctrl ? ctrl->process_secondary(data, len) : 0;
}

bool XboxWirelessController::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    return m_adapter->xfer_cb(ep_addr, result, xferred_bytes);
}

bool XboxWirelessController::set_config()
{
    return m_adapter->set_config();
}

static void wireless_on_device_descriptor_wrapper(void *context, SubType subtype)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl && subtype != Unknown) {
        ctrl->on_device_descriptor(subtype);
    }
}

void XboxWirelessController::on_device_descriptor(SubType subtype)
{
    m_controller.device_type = (xbox_device_type_t)subtype;
    m_controller.gip_device.subtype = subtype;
    m_subtype = subtype;
    m_controller.status = XBOX_CONTROLLER_READY;

    // No USB product string for a wireless controller, so build one (UTF-16LE string descriptor)
    // from the GIP descriptor's display name for the config tool
    memset(m_name, 0, sizeof(m_name));
    if (const char *name = m_controller.gip_device.display_name)
    {
        size_t len = strnlen(name, sizeof(m_name) / 2 - 1);
        m_name[0] = (len + 1) * 2;
        m_name[1] = TUSB_DESC_STRING;
        for (size_t i = 0; i < len; i++)
        {
            m_name[(i + 1) * 2] = name[i];
        }
    }

    usb_host_add_assignable_interface(m_adapter->get_controller_interface(m_controller_idx));
    gip_send_power_on_sequence(&m_controller.gip_device);
    process_delayed_init();
    send_hotplug_event(true);
    if (!auth_broker.has_handler(ModeXboxOne))
    {
        auth_broker.register_handler(ModeXboxOne, [this](XGIPProtocol* packet) {
            this->send_report_from_host(packet);
        });
        m_auth_registered = true;
    }
    printf("Wireless controller %d identified as type %d\n", m_controller_idx, (uint8_t)subtype);
}

static void wireless_on_arrival_wrapper(void *context)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl) {
        ctrl->m_controller.status = XBOX_CONTROLLER_CONNECTED;
        gip_default_arrival_callback(&ctrl->m_controller.gip_device, wireless_queue_packet_wrapper);
    }
}

int XboxWirelessController::send_gip_packet(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0)
    {
        return -1;
    }

    uint8_t wcid = m_controller_idx + 1;
    
    m_adapter->send_report_from_host(wcid, m_controller.mac_addr, data, len);
    return 0;
}

int XboxWirelessController::send_ack_packet(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0)
    {
        return -1;
    }

    uint8_t wcid = m_controller_idx + 1;
    
    m_adapter->send_ack_from_host(wcid, m_controller.mac_addr, data, len);
    return 0;
}

static void wireless_queue_packet_wrapper(void *context, const uint8_t *data, uint16_t len)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl) {
        ctrl->send_gip_packet(data, len);
    }
}

static void wireless_send_ack_wrapper(void *context, const uint8_t *data, uint16_t len)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl) {
        ctrl->send_ack_packet(data, len);
    }
}

static void wireless_on_disconnect_wrapper(void *context)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl) {
        ctrl->on_disconnect();
    }
}

void XboxWirelessController::on_disconnect()
{
    m_controller.status = XBOX_CONTROLLER_DISCONNECTED;
    still_connected = false;
    if (m_adapter)
    {
        m_adapter->disconnect_controller(m_controller_idx);
    }
}

void XboxWirelessController::process_gip_data(const uint8_t *data, uint16_t len)
{
    if (len < 4) {
        printf("RX GIP to controller %d: len=%d (too short)\n", m_controller_idx, len);
        return;
    }
    
    xbox_controller_t *controller = get_controller_data();
    if (!controller) {
        printf("RX GIP to controller %d: no controller data\n", m_controller_idx);
        return;
    }
    
    gip_device_process_incoming(&controller->gip_device, data, len);
}

void XboxWirelessController::set_rumble(uint8_t left, uint8_t right)
{
    xbox_controller_t *controller = get_controller_data();
    if (!controller || controller->status != XBOX_CONTROLLER_READY)
    {
        return;
    }
    GipRumble_t rumble = {};
    // Motor bitmap ([MS-GIPUSB] Table 56): right / left vibration (0x01 / 0x02) carry the levels,
    // right / left impulse (0x04 / 0x08) are left at 0
    rumble.flags = 0x0F;
    rumble.leftMotor = (uint16_t)left * 100 / 255;
    rumble.rightMotor = (uint16_t)right * 100 / 255;
    rumble.duration = (left || right) ? 0xFF : 0;

    send_feedback_packet(GIP_CMD_RUMBLE, false, (uint8_t *)&rumble, sizeof(rumble));
}

// Feedback must not touch gip_device.outgoing_xgip: it may hold an in-flight
// chunked auth transfer forwarded from the console, and resetting it mid-transfer
// truncates the auth exchange.
void XboxWirelessController::send_feedback_packet(uint8_t command, bool system, const uint8_t *data, uint16_t len)
{
    static XGIPProtocol feedback;
    feedback.reset();
    feedback.setAttributes(command, gip_sequence_pool_next(&m_controller.gip_device.tx_sequence_pools, command), system, 0, 0);
    feedback.setData(data, len);
    uint8_t *packet = feedback.generatePacket();
    m_adapter->send_report_from_host(m_controller_idx + 1, m_controller.mac_addr, packet, feedback.getPacketLength());
}

void XboxWirelessController::set_player_led(uint8_t player)
{
    xbox_controller_t *controller = get_controller_data();
    if (!controller || controller->status != XBOX_CONTROLLER_READY)
    {
        return;
    }
    uint8_t data[3] = {0x00, (uint8_t)(player ? 0x01 : 0x00), 0x14};
    // LED is a system message ([MS-GIPUSB] Table 41)
    send_feedback_packet(GIP_CMD_LED_ON, true, data, sizeof(data));
}

