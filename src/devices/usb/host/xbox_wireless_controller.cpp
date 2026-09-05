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

static const gip_device_interface_t wireless_gip_interface = {
    .on_device_descriptor = wireless_on_device_descriptor_wrapper,
    .on_arrival = wireless_on_arrival_wrapper,
    .queue_packet = wireless_queue_packet_wrapper
};

XboxWirelessController::XboxWirelessController(XboxWirelessHost* adapter, uint8_t controller_idx, uint8_t dev_addr, uint16_t id)
    : UsbHostInterface(dev_addr, controller_idx, id)
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
    m_controller.gip_device.outgoing_xgip->copyAttributes(report);
    m_adapter->send_report_from_host(m_controller_idx + 1, m_controller.mac_addr,
                                     m_controller.gip_device.outgoing_xgip->generatePacket(),
                                     m_controller.gip_device.outgoing_xgip->getPacketLength());
}
bool XboxWirelessController::tick_digital(proto_Output &type)
{
    xbox_controller_t* controller = get_controller_data();
    
    if (!controller || controller->status != XBOX_CONTROLLER_READY)
    {
        return false;
    }
    
    return gip_tick_digital(controller->gip_device.raw_input, m_subtype, &type);
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
        
    usb_host_add_assignable_interface(m_adapter->get_controller_interface(m_controller_idx));
    gip_send_power_on_sequence(&m_controller.gip_device);
    process_delayed_init();
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
    
    return m_adapter->send_gip_to_controller(wcid, m_controller.mac_addr, data, len);
}

static void wireless_queue_packet_wrapper(void *context, const uint8_t *data, uint16_t len)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl) {
        ctrl->send_gip_packet(data, len);
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

    uint8_t *ack_data;
    uint16_t ack_len;
    if (gip_device_generate_ack(&controller->gip_device, &ack_data, &ack_len))
    {
        m_adapter->send_ack_from_host(m_controller_idx + 1, controller->mac_addr, ack_data, ack_len);
    }
}
