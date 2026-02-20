#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"

std::shared_ptr<UsbHostInterface> XInputModuleHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, nullptr);

    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x02)
    {
        p_desc = tu_desc_next(p_desc);
        // TODO: this
        auto intf = std::make_shared<XInputModuleHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
        return intf;
    }
    return nullptr;
}
