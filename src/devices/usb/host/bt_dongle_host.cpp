#include "tusb_option.h"
#include "devices/usb/host/bt_dongle_host.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"

// Bluetooth Core spec, Vol 4 Part B: the primary controller interface
#define BT_SUBCLASS_RF 0x01
#define BT_PROTOCOL_PRIMARY_CONTROLLER 0x01

#define REALTEK_VID 0x0bda
#define INTEL_VID 0x8087

// only one adapter is used at a time
static BtDongleHost *s_dongle = nullptr;

bool usb_dongle_present()
{
    return s_dongle != nullptr;
}

bool usb_dongle_can_send(uint8_t packet_type)
{
    return s_dongle && s_dongle->can_send(packet_type);
}

bool usb_dongle_send(uint8_t packet_type, const uint8_t *packet, uint16_t len)
{
    return s_dongle && s_dongle->send(packet_type, packet, len);
}

BtDongleHost::~BtDongleHost()
{
    if (s_dongle == this)
    {
        s_dongle = nullptr;
    }
}

std::shared_ptr<UsbHostInterface> BtDongleHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    // Interface 0 has the events and ACL data, interface 1 is SCO audio, which isn't used
    TU_VERIFY(desc_itf->bInterfaceClass == TUSB_CLASS_WIRELESS_CONTROLLER &&
                  desc_itf->bInterfaceSubClass == BT_SUBCLASS_RF &&
                  desc_itf->bInterfaceProtocol == BT_PROTOCOL_PRIMARY_CONTROLLER &&
                  desc_itf->bInterfaceNumber == 0 && desc_itf->bAlternateSetting == 0,
              nullptr);
    uint8_t dev_addr = list->dev_addr();
    if (s_dongle)
    {
        printf("BT dongle: already using a bluetooth adapter, ignoring the one at %d\r\n", dev_addr);
        return nullptr;
    }
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    printf("BT dongle: found bluetooth adapter %04x:%04x\r\n", vid, pid);
    if (vid == REALTEK_VID || vid == INTEL_VID)
    {
        // these need firmware uploaded before they will work, which isn't supported yet
        printf("BT dongle: Realtek and Intel adapters need firmware we don't load yet, this adapter probably won't work. CSR and Broadcom adapters do.\r\n");
    }

    auto intf = std::make_shared<BtDongleHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    uint16_t size = desc_itf->bLength;
    const uint8_t *p_desc = (const uint8_t *)desc_itf;
    const uint8_t *desc_end = p_desc + max_len;
    uint8_t endpoints = desc_itf->bNumEndpoints;
    while (endpoints)
    {
        p_desc = tu_desc_next(p_desc);
        TU_VERIFY(tu_desc_in_bounds(p_desc, desc_end) && tu_desc_len(p_desc) != 0, nullptr);
        size += tu_desc_len(p_desc);
        if (tu_desc_type(p_desc) != TUSB_DESC_ENDPOINT)
        {
            continue;
        }
        endpoints--;
        const tusb_desc_endpoint_t *desc_ep = (const tusb_desc_endpoint_t *)p_desc;
        uint8_t ep = desc_ep->bEndpointAddress;
        bool in = ep & 0x80;
        switch (desc_ep->bmAttributes.xfer)
        {
        case TUSB_XFER_INTERRUPT:
            if (in)
            {
                intf->m_ep_event = ep;
                intf->m_event_packet_size = tu_edpt_packet_size(desc_ep);
            }
            break;
        case TUSB_XFER_BULK:
            if (in)
            {
                intf->m_ep_acl_in = ep;
                intf->m_acl_in_packet_size = tu_edpt_packet_size(desc_ep);
            }
            else
            {
                intf->m_ep_acl_out = ep;
            }
            break;
        default:
            continue;
        }
        TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        if (in)
        {
            list->host_devices_by_endpoint_in[ep & ~0x80] = intf;
        }
        else
        {
            list->host_devices_by_endpoint_out[ep] = intf;
        }
    }
    if (!intf->m_ep_event || !intf->m_ep_acl_in || !intf->m_ep_acl_out)
    {
        printf("BT dongle: adapter is missing an endpoint\r\n");
        return nullptr;
    }
    *out_len = size;
    return intf;
}

bool BtDongleHost::set_config()
{
    // No product string or hotplug event, as this isn't an input device
    still_connected = true;
    tuh_vid_pid_get(m_dev_addr, &m_vid, &m_pid);
    usbh_driver_set_config_complete(m_dev_addr, m_interface);
    s_dongle = this;
    m_event_len = 0;
    m_acl_len = 0;
    m_command_busy = false;
    m_acl_out_busy = false;
    receive_event();
    receive_acl();
    usb_dongle_attached();
    return true;
}

void BtDongleHost::disconnect()
{
    UsbHostInterface::disconnect();
    if (s_dongle == this)
    {
        s_dongle = nullptr;
        usb_dongle_detached();
    }
}

// Ask for the next USB packet of the event being received. Once the header is in, only ask for what is
// left of the event, so the adapter never sends more than the buffer has room for.
void BtDongleHost::receive_event()
{
    uint16_t want = m_event_packet_size;
    if (m_event_len >= 2)
    {
        want = TU_MIN(want, 2 + m_event_buf[1] - m_event_len);
    }
    want = TU_MIN(want, sizeof(m_event_buf) - m_event_len);
    usbh_edpt_xfer(m_dev_addr, m_ep_event, m_event_buf + m_event_len, want);
}

void BtDongleHost::receive_acl()
{
    uint16_t want = m_acl_in_packet_size;
    if (m_acl_len >= 4)
    {
        uint16_t total = 4 + tu_le16toh(tu_unaligned_read16(m_acl_buf + 2));
        want = TU_MIN(want, total - m_acl_len);
    }
    want = TU_MIN(want, sizeof(m_acl_buf) - m_acl_len);
    usbh_edpt_xfer(m_dev_addr, m_ep_acl_in, m_acl_buf + m_acl_len, want);
}

bool BtDongleHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (s_dongle != this)
    {
        return true;
    }
    if (ep_addr == m_ep_acl_out)
    {
        m_acl_out_busy = false;
        usb_dongle_packet_sent();
        return true;
    }
    if (ep_addr == m_ep_event)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            m_event_len += xferred_bytes;
            if (m_event_len >= 2 && m_event_len >= 2 + m_event_buf[1])
            {
                uint16_t len = 2 + m_event_buf[1];
                m_event_len = 0;
                usb_dongle_packet_received(USB_DONGLE_EVENT_PACKET, m_event_buf, len);
            }
            else if (xferred_bytes < m_event_packet_size)
            {
                // a short packet ends the transfer, so whatever this was it isn't coming
                m_event_len = 0;
            }
        }
        else
        {
            m_event_len = 0;
        }
        if (s_dongle == this)
        {
            receive_event();
        }
        return true;
    }
    if (ep_addr == m_ep_acl_in)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            m_acl_len += xferred_bytes;
            uint16_t total = m_acl_len >= 4 ? 4 + tu_le16toh(tu_unaligned_read16(m_acl_buf + 2)) : 0;
            if (total > sizeof(m_acl_buf))
            {
                printf("BT dongle: ACL packet too big (%u)\r\n", total);
                m_acl_len = 0;
            }
            else if (total && m_acl_len >= total)
            {
                m_acl_len = 0;
                usb_dongle_packet_received(USB_DONGLE_ACL_PACKET, m_acl_buf, total);
            }
            else if (xferred_bytes < m_acl_in_packet_size)
            {
                m_acl_len = 0;
            }
        }
        else
        {
            m_acl_len = 0;
        }
        if (s_dongle == this)
        {
            receive_acl();
        }
        return true;
    }
    return true;
}

bool BtDongleHost::can_send(uint8_t packet_type)
{
    switch (packet_type)
    {
    case USB_DONGLE_COMMAND_PACKET:
        return !m_command_busy;
    case USB_DONGLE_ACL_PACKET:
        return !m_acl_out_busy;
    default:
        return false;
    }
}

void BtDongleHost::command_complete(tuh_xfer_t *xfer)
{
    BtDongleHost *dongle = (BtDongleHost *)xfer->user_data;
    if (s_dongle != dongle)
    {
        return;
    }
    if (xfer->result != XFER_RESULT_SUCCESS)
    {
        printf("BT dongle: sending a command failed (%d)\r\n", xfer->result);
    }
    dongle->m_command_busy = false;
    usb_dongle_packet_sent();
}

bool BtDongleHost::send(uint8_t packet_type, const uint8_t *packet, uint16_t len)
{
    if (!can_send(packet_type))
    {
        return false;
    }
    switch (packet_type)
    {
    case USB_DONGLE_COMMAND_PACKET:
    {
        if (len > sizeof(m_command_buf))
        {
            return false;
        }
        memcpy(m_command_buf, packet, len);
        // HCI commands are a class request to the device, with no other parameters
        m_command_request = {
            .bmRequestType_bit = {
                .recipient = TUSB_REQ_RCPT_DEVICE,
                .type = TUSB_REQ_TYPE_CLASS,
                .direction = TUSB_DIR_OUT},
            .bRequest = 0,
            .wValue = 0,
            .wIndex = 0,
            .wLength = len};
        tuh_xfer_t xfer = {};
        xfer.daddr = m_dev_addr;
        xfer.ep_addr = 0;
        xfer.setup = &m_command_request;
        xfer.buffer = m_command_buf;
        xfer.complete_cb = command_complete;
        xfer.user_data = (uintptr_t)this;
        m_command_busy = true;
        if (!tuh_control_xfer(&xfer))
        {
            m_command_busy = false;
            return false;
        }
        return true;
    }
    case USB_DONGLE_ACL_PACKET:
        // BTstack keeps the packet around until it is told it was sent, so it can be sent from directly
        if (!usbh_edpt_claim(m_dev_addr, m_ep_acl_out))
        {
            return false;
        }
        m_acl_out_busy = true;
        if (!usbh_edpt_xfer(m_dev_addr, m_ep_acl_out, (uint8_t *)packet, len))
        {
            m_acl_out_busy = false;
            usbh_edpt_release(m_dev_addr, m_ep_acl_out);
            return false;
        }
        return true;
    default:
        return false;
    }
}
