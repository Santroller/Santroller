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
    if (!auth_broker.has_handler(ModeXboxOne))
    {
        auth_broker.register_handler(ModeXboxOne, [this](XGIPProtocol* packet) {
            this->send_report_from_host(packet);
        });
        m_auth_registered = true;
    }
    
    printf("XboxWirelessController: Created virtual interface for controller %d\r\n", controller_idx);
}

XboxWirelessController::~XboxWirelessController()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModeXboxOne);
        m_auth_registered = false;
    }
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
    m_adapter->send_report_from_host(m_controller.gip_device.outgoing_xgip->generatePacket(), 
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
        uint8_t controller_idx = *(uint8_t *)context;
        xbox_adapter_send_gip_packet(controller_idx, data, len);
    };
    gip_device_update(&controller->gip_device, now, XGIP_ACK_WAIT_TIMEOUT, queue_lambda, &m_controller_idx);
}

bool XboxWirelessController::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    return true;
}

bool XboxWirelessController::set_config()
{
    return true;
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
        
    // Move from enumerating to assignable
    DeviceManager::instance().add_assignable_usb_device(host_devices[m_adapter->dev_addr()]->host_devices_by_itf[m_adapter->interface()]);
    process_delayed_init();
    printf("Wireless controller %d identified as type %d\n", m_controller_idx, (uint8_t)subtype);
}

static void wireless_on_arrival_wrapper(void *context)
{
    XboxWirelessController *ctrl = (XboxWirelessController *)context;
    
    if (ctrl) {
        ctrl->m_controller.status = XBOX_CONTROLLER_CONNECTED;
        printf("Controller %d connected\n", ctrl->m_controller_idx);
        
        gip_default_arrival_callback(&ctrl->m_controller.gip_device, wireless_queue_packet_wrapper);
    }
}

int XboxWirelessController::send_gip_packet(const uint8_t *data, uint16_t len)
{
    printf("TX GIP to controller %d: cmd=0x%02X, len=%d\n", 
           m_controller_idx, data[0], len);
    
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
        return;
    }
    
    xbox_controller_t *controller = get_controller_data();
    if (!controller) {
        return;
    }
    
    gip_device_process_incoming(&controller->gip_device, data, len);
}
