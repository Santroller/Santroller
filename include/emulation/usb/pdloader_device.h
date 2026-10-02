#pragma once
#include "emulation/usb/device.hpp"
#include "protocols/pdloader.hpp"

class PDLoaderDevice : public UsbDevice {
public:
    void initialize() override;
    void process(bool full_poll, bool send_events) override;
    size_t compatible_section_descriptor(uint8_t *, size_t) override { return 0; }
    size_t config_descriptor(uint8_t *dest, size_t remaining) override;
    size_t device_name(uint8_t idx, char *desc) override;
    void device_descriptor(tusb_desc_device_t *desc) override;
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t bytes) override;
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request) override;
    uint16_t open(tusb_desc_interface_t const *desc, uint16_t max_len) override;

    static const uint8_t *bos_descriptor();
    static const uint8_t *ms_os_20_descriptor();
    static uint16_t ms_os_20_descriptor_length();
    const uint8_t *led_report() const { return m_led_complete ? m_led_report : nullptr; }

private:
    uint8_t m_epin = 0;
    uint8_t m_epout = 0;
    uint8_t m_strid = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_epin_buffer[PDLOADER_PACKET_SIZE] = {};
    CFG_TUSB_MEM_ALIGN uint8_t m_epout_buffer[PDLOADER_PACKET_SIZE] = {};
    uint8_t m_led_staging[PDLOADER_LED_REPORT_SIZE] = {};
    uint8_t m_led_report[PDLOADER_LED_REPORT_SIZE] = {};
    uint8_t m_led_offset = 0;
    bool m_led_complete = false;
    PDLoaderInputReport m_input;
};
