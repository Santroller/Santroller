#include "tusb_option.h"
#include "usb/host/xone_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "devices/usb.hpp"
#include "emulation/usb/hid_device.h"
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
    // {WirelessLegacyAdapter, "MadCatz.Xbox.Module.Brangus"},
    // {WiredLegacyAdapter, "PDP.Xbox.RBAdapter.LegacyUSB"},
    {Skylanders, "Activision.Xbox.Skylanders.Portal"},
    {LegoDimensions, "TTGames.Xbox.Dimensions.Gateway"},
    {DisneyInfinity, "Disney.Xbox.Infinity.Base"}};
XboxOneHost::XboxOneHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id)
{
    m_delayed_init = true;
    incomingXGIP = new XGIPProtocol();
    outgoingXGIP = new XGIPProtocol();
}

std::shared_ptr<UsbHostInterface> XboxOneHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    printf("XboxOneHost::open\r\n");
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

        if (incomingXGIP->ackRequired())
        {
            queue_xbone_report(incomingXGIP->generateAckPacket(), incomingXGIP->getPacketLength());
        }
        if (incomingXGIP->getCommand() == GIP_DEVICE_DESCRIPTOR && incomingXGIP->endOfChunk())
        {
            uint8_t *data = incomingXGIP->getData();
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
                                                                     { return intf.get() == this; }),
                                                      enumerating_usb_devices.end());
                        assignable_usb_devices.push_back(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
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
            process_delayed_init();
        }
        if (incomingXGIP->getCommand() == GIP_INPUT_REPORT)
        {
            memcpy(m_last_inputs, incomingXGIP->getData(), incomingXGIP->getDataLength());
        }
        if (incomingXGIP->getCommand() == GIP_ARRIVAL)
        {
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
            printf("sent\r\n");
            report_queue.pop();
        }
    }
}

bool XboxOneHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (XboxOneGamepad_Data_t *)m_last_inputs;
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:
            return data->a;
        case Gamepad_B:
            return data->b;
        case Gamepad_X:
            return data->x;
        case Gamepad_Y:
            return data->y;
        case Gamepad_LeftShoulder:
            return data->leftShoulder;
        case Gamepad_RightShoulder:
            return data->rightShoulder;
        case Gamepad_Back:
            return data->back;
        case Gamepad_Start:
            return data->start;
        case Gamepad_LeftThumbClick:
            return data->leftThumbClick;
        case Gamepad_RightThumbClick:
            return data->rightThumbClick;
        case Gamepad_Guide:
            return data->guide;
        case Gamepad_DpadUp:
            return data->dpadUp;
        case Gamepad_DpadDown:
            return data->dpadDown;
        case Gamepad_DpadLeft:
            return data->dpadLeft;
        case Gamepad_DpadRight:
            return data->dpadRight;
        default:
            return false;
        }
    }
    switch (m_subtype)
    {
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbButton_tag)
        {
            auto data = (XboxOneRockBandGuitar_Data_t *)m_last_inputs;
            switch (type.mapping.rbButton)
            {
            case RockBandGuitar_Green:
                return data->green;
            case RockBandGuitar_Red:
                return data->red;
            case RockBandGuitar_Yellow:
                return data->yellow;
            case RockBandGuitar_Blue:
                return data->blue;
            case RockBandGuitar_Orange:
                return data->orange;
            case RockBandGuitar_SoloGreen:
                return data->soloGreen;
            case RockBandGuitar_SoloRed:
                return data->soloRed;
            case RockBandGuitar_SoloYellow:
                return data->soloYellow;
            case RockBandGuitar_SoloBlue:
                return data->soloBlue;
            case RockBandGuitar_SoloOrange:
                return data->soloOrange;
            default:
                return false;
            }
        }
        return false;
    case RockBandDrums:

        if (type.which_mapping == proto_Output_rbDrumButton_tag)
        {
            auto data = (XboxOneRockBandDrums_Data_t *)m_last_inputs;
            switch (type.mapping.rbDrumButton)
            {
            default:
                return false;
            }
        }
        return false;
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlButton_tag)
        {
            auto data = (XboxOneGHLGuitar_Data_t *)m_last_inputs;
            switch (type.mapping.ghlButton)
            {
            case GuitarHeroLiveGuitar_Black1:
                return data->report.a;
            case GuitarHeroLiveGuitar_Black2:
                return data->report.b;
            case GuitarHeroLiveGuitar_Black3:
                return data->report.y;
            case GuitarHeroLiveGuitar_White1:
                return data->report.x;
            case GuitarHeroLiveGuitar_White2:
                return data->report.leftShoulder;
            case GuitarHeroLiveGuitar_White3:
                return data->report.rightShoulder;
            case GuitarHeroLiveGuitar_StrumUp:
                return data->report.strumBar == 0x00;
            case GuitarHeroLiveGuitar_StrumDown:
                return data->report.strumBar == 0xFF;
            default:
                return false;
            }
        }
        return false;
    default:
        return false;
    }

    return false;
}
uint16_t XboxOneHost::tick_analog(proto_Output &type)
{
    switch (m_subtype)
    {
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (XboxOneGHLGuitar_Data_t *)m_last_inputs;
            switch (type.mapping.ghlAxis)
            {
            case GuitarHeroLiveGuitar_Whammy:
                return data->report.whammy << 8;
            case GuitarHeroLiveGuitar_Tilt:
                return data->report.tilt << 2;
            default:
                return 0;
            }
        }
        break;
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbAxis_tag)
        {
            auto data = (XboxOneRockBandGuitar_Data_t *)m_last_inputs;
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return data->whammy + INT16_MAX;
            case RockBandGuitar_Tilt:
                return data->tilt + INT16_MAX;
            case RockBandGuitar_Pickup:
                return data->tilt + INT16_MAX;
            default:
                return 0;
            }
        }
    default:
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            auto data = (XboxOneGamepad_Data_t *)m_last_inputs;
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftTrigger:
                return data->leftTrigger << 8;
            case Gamepad_RightTrigger:
                return data->rightTrigger << 8;
            case Gamepad_LeftStickX:
                return data->leftStickX + INT16_MAX;
            case Gamepad_LeftStickY:
                return data->leftStickY + INT16_MAX;
            case Gamepad_RightStickX:
                return data->rightStickX + INT16_MAX;
            case Gamepad_RightStickY:
                return data->rightStickY + INT16_MAX;
            default:
                return 0;
            }
        }
        break;
    }
    return 0;
}