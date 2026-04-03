#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_descriptors.h"

std::shared_ptr<UsbHostInterface> XInputAudioHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t* out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x03)
    {
        p_desc = tu_desc_next(p_desc);
        *out_len = TUD_XINPUT_AUDIO_DESC_LEN;
        // TODO: this
        return std::make_shared<XInputAudioHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    }
    return nullptr;
}