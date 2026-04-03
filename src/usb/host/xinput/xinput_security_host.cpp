#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_descriptors.h"

std::shared_ptr<UsbHostInterface> XInputSecurityHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t* out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, nullptr);

    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    if (desc_itf->bInterfaceSubClass == 0xfD &&
        desc_itf->bInterfaceProtocol == 0x13)
    {
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, nullptr);
        auto intf = std::make_shared<XInputSecurityHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
        p_desc = tu_desc_next(p_desc);
        *out_len = TUD_XINPUT_SECURITY_DESC_LEN;
        return intf;
    }
    return nullptr;
}