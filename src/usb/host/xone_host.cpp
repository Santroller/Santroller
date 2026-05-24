#include "tusb_option.h"
#include "usb/host/xone_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "devices/usb.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
#include "utils.h"
#include <algorithm>
static const uint8_t XBOXONE_POWER_ON[] = {0x06, 0x62, 0x45, 0xb8, 0x77, 0x26, 0x2c, 0x55,
                                           0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f};
static const uint8_t XBOXONE_POWER_ON_SINGLE[] = {0x00};
static const uint8_t XBOXONE_RUMBLE_ON[] = {0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xeb};
static const uint8_t XBOXONE_LED_ON[] = {0x00, 0x01, 0x14}; // 0x01 - LED on, 0x14 - Brightness
typedef struct
{
    SubType type;
    char name[36];
} preferred_type_mapping_t;
static const preferred_type_mapping_t PREFERRED_TYPES[] = {
    {Gamepad, "Windows.Xbox.Input.Gamepad"},
    {RockBandGuitar, "MadCatz.Xbox.Guitar.Stratocaster"},
    {RockBandGuitar, "PDP.Xbox.Guitar.Jaguar"},
    {LiveGuitar, "Activision.Xbox.Input.GH7"},
    {RockBandDrums, "MadCatz.Xbox.Drums.Glam"},
    {RockBandDrums, "PDP.Xbox.Drums.Tablah"},
    {WirelessLegacyAdapter, "MadCatz.Xbox.Module.Brangus"},
    {WiredLegacyAdapter, "PDP.Xbox.RBAdapter.LegacyUSB"},
    {Skylanders, "Activision.Xbox.Skylanders.Portal"},
    {LegoDimensions, "TTGames.Xbox.Dimensions.Gateway"},
    {DisneyInfinity, "Disney.Xbox.Infinity.Base"}};
XboxOneHost::XboxOneHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id)
{
    incomingXGIP = new XGIPProtocol();
    outgoingXGIP = new XGIPProtocol();
}

std::shared_ptr<UsbHostInterface> XboxOneHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    uint32_t size = desc_itf->bLength;
    TU_VERIFY(desc_itf->bInterfaceSubClass == 0x47 &&
                  desc_itf->bInterfaceProtocol == 0xD0 && desc_itf->bAlternateSetting == 0,
              nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XboxOneHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    intf->m_subtype = SubType_Gamepad;
    uint8_t endpoints = desc_itf->bNumEndpoints;
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        size += desc_ep->bLength;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);

        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
            usbh_edpt_xfer(dev_addr, intf->m_ep_in, intf->m_ep_in_buf, intf->m_ep_in_size);
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
    }
    if (intf->m_ep_out)
    {
        list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
    }
    if (intf->m_ep_in)
    {
        list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    }
    if (desc_itf->bInterfaceNumber == 0)
    {
        enumerating_usb_devices.push_back(intf);
    }
    printf("size: %d\r\n", size);
    *out_len = size;
    return intf;
}

bool XboxOneHost::set_config()
{
    printf("set config\r\n");
    memset(m_last_inputs, 0, sizeof(m_last_inputs));
    UsbHostInterface::set_config();
    return true;
}
void XboxOneHost::queue_xbone_report(void *report, uint16_t report_size)
{
    report_queue_t item;
    memcpy(item.report, report, report_size);
    item.len = report_size;
    report_queue.push(item);
}

bool XboxOneHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80 && result != XFER_RESULT_FAILED)
    {
        if (xferred_bytes == 0)
        {
            usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
            return true;
        }
        incomingXGIP->parse(m_ep_in_buf, xferred_bytes);

        printf("cmd: %02x %02x %02x %02x\r\n", incomingXGIP->getCommand(), incomingXGIP->ackRequired(), incomingXGIP->endOfChunk(), xferred_bytes);
        if (incomingXGIP->ackRequired())
        {
            printf("send ack!\r\n");
            queue_xbone_report(incomingXGIP->generateAckPacket(), incomingXGIP->getPacketLength());
        }
        if (incomingXGIP->getCommand() == GIP_DEVICE_DESCRIPTOR && incomingXGIP->endOfChunk())
        {
            uint8_t *data = incomingXGIP->getData();
            BinaryMetadataHeader *header = (BinaryMetadataHeader *)data;
            data = incomingXGIP->getData();
            data += sizeof(BinaryMetadataHeader);
            printf("descriptor read done!\r\n");
            BinaryDeviceMetadata *metadata = (BinaryDeviceMetadata *)data;
            data += metadata->preferred_types_offset;
            // First byte at the offset is a count of items
            uint8_t preferredTypeStrCount = *data++;
            bool found = false;
            for (size_t j = 0; j < preferredTypeStrCount; j++)
            {
                if (found)
                {
                    break;
                }
                // first two bytes are the string length
                uint16_t len = *(uint16_t *)data;
                data += 2;
                // check if we know what device this is
                for (size_t i = 0; i < TU_ARRAY_SIZE(PREFERRED_TYPES); i++)
                {
                    if (strncmp((char *)data, PREFERRED_TYPES[i].name, len) == 0)
                    {
                        // we found it, flag the device as assignable and reload
                        m_subtype = PREFERRED_TYPES[i].type;
                        printf("found subtype: %d\r\n", m_subtype);
                        enumerating_usb_devices.erase(std::remove_if(enumerating_usb_devices.begin(), enumerating_usb_devices.end(), [this](std::shared_ptr<UsbHostInterface> intf)
                                                                     { return intf.get() == this; }));
                        assignable_usb_devices.push_back(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                        if (HIDConfigDevice::tool_closed())
                        {
                            reload();
                        }
                        found = true;
                        break;
                    }
                }
            }
            outgoingXGIP->reset();
            outgoingXGIP->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, 2, 1, 0, 0);
            outgoingXGIP->setData(XBOXONE_POWER_ON, sizeof(XBOXONE_POWER_ON));
            queue_xbone_report(outgoingXGIP->generatePacket(), outgoingXGIP->getPacketLength());
            outgoingXGIP->reset();
            outgoingXGIP->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, 3, 1, 0, 0);
            outgoingXGIP->setData(XBOXONE_POWER_ON_SINGLE, sizeof(XBOXONE_POWER_ON_SINGLE));
            queue_xbone_report(outgoingXGIP->generatePacket(), outgoingXGIP->getPacketLength());
            outgoingXGIP->reset();
            outgoingXGIP->setAttributes(GIP_CMD_LED_ON, 1, 1, 0, 0);
            outgoingXGIP->setData(XBOXONE_LED_ON, sizeof(XBOXONE_LED_ON));
            queue_xbone_report(outgoingXGIP->generatePacket(), outgoingXGIP->getPacketLength());
            outgoingXGIP->reset();
            outgoingXGIP->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, 1, 1, 0, 0);
            outgoingXGIP->setData(XBOXONE_RUMBLE_ON, sizeof(XBOXONE_RUMBLE_ON));
            queue_xbone_report(outgoingXGIP->generatePacket(), outgoingXGIP->getPacketLength());
        }
        if (incomingXGIP->getCommand() == GIP_INPUT_REPORT)
        {
            memcpy(m_last_inputs, incomingXGIP->getData(), incomingXGIP->getDataLength());
        }
        if (incomingXGIP->getCommand() == GIP_ARRIVAL)
        {
            printf("send metadata!\r\n");
            outgoingXGIP->reset();
            outgoingXGIP->setAttributes(GIP_DEVICE_DESCRIPTOR, 1, 1, false, 0);
            queue_xbone_report(outgoingXGIP->generatePacket(), outgoingXGIP->getPacketLength());
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void XboxOneHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (!report_queue.empty())
    {
        if (send_intr_xfer(m_ep_out, &report_queue.front().report, report_queue.front().len))
        {
            report_queue.pop();
        }
    }
}

bool XboxOneHost::tick_digital(UsbButtonType type)
{
    switch (m_subtype)
    {
    case Gamepad:
        switch (type)
        {
        case UsbButtonA:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->a;
        case UsbButtonB:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->b;
        case UsbButtonX:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->x;
        case UsbButtonY:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->y;
        case UsbButtonLeftShoulder:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->leftShoulder;
        case UsbButtonRightShoulder:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->rightShoulder;
        case UsbButtonBack:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->back;
        case UsbButtonStart:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->start;
        case UsbButtonGuide:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->guide;
        case UsbButtonLeftThumbClick:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->leftThumbClick;
        case UsbButtonRightThumbClick:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->rightThumbClick;
        case UsbButtonDpadUp:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->dpadUp;
        case UsbButtonDpadDown:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->dpadRight;
        default:
            return false;
        }
        break;
    case RockBandGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->a;
        case UsbButtonRed:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->b;
        case UsbButtonYellow:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->y;
        case UsbButtonBlue:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->x;
        case UsbButtonOrange:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->leftShoulder;
        case UsbButtonBack:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->back;
        case UsbButtonStart:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->start;
        case UsbButtonGuide:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->guide;
        case UsbButtonStrumUp:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->dpadUp;
        case UsbButtonStrumDown:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->dpadRight;
        default:
            return false;
        }
        break;
    }

    return false;
}
uint16_t XboxOneHost::tick_analog(UsbAxisType type)
{
    switch (m_subtype)
    {
    case Gamepad:
        switch (type)
        {
        case UsbAxisLeftTrigger:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->leftTrigger << 8;
        case UsbAxisRightTrigger:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->rightTrigger << 8;
        case UsbAxisLeftStickX:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->leftStickX + INT16_MAX;
        case UsbAxisLeftStickY:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->leftStickY + INT16_MAX;
        case UsbAxisRightStickX:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->rightStickX + INT16_MAX;
        case UsbAxisRightStickY:
            return ((XboxOneGamepad_Data_t *)m_last_inputs)->rightStickY + INT16_MAX;
        default:
            return 0;
        }
        break;
    case RockBandGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->whammy << 8;
        case UsbAxisTilt:
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->tilt << 8;
        case UsbAxisPickup:
            // TODO: map this right
            return ((XboxOneRockBandGuitar_Data_t *)m_last_inputs)->pickup + INT16_MAX;
        default:
            return 0;
        }
        break;
    }
    return 0;
}