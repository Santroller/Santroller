#pragma once

#include "common/tusb_common.h"
#include "device/usbd.h"
#include "device.hpp"
#include <queue>

#ifndef CFG_TUD_XONE_EPSIZE
#define CFG_TUD_XONE_EPSIZE 64
#endif

typedef enum
{
    READY_ANNOUNCE,
    WAIT_DESCRIPTOR_REQUEST,
    SEND_DESCRIPTOR,
    SETUP_AUTH,
    AUTH_DONE,
    NOT_READY
} XboxOneDriverState;

typedef struct
{
    uint8_t report[CFG_TUD_XONE_TX_BUFSIZE];
    uint16_t len;
} report_queue_t;

class XboxOneGamepadDevice : public UsbDevice
{
public:
    XboxOneGamepadDevice();
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    void process_report_queue(uint32_t now);
    bool send_xbone_usb(uint8_t const *report, uint16_t report_size);
    void queue_xbone_report(void *report, uint16_t report_size);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    void set_ack_wait();
    uint8_t m_epin;
    uint8_t m_epout;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XONE_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XONE_RX_BUFSIZE];

private:
    uint8_t last_report[CFG_TUD_XONE_RX_BUFSIZE] = {};
    uint8_t last_report_counter;
    uint32_t keep_alive_timer;
    uint8_t keep_alive_sequence;
    uint8_t virtual_keycode_sequence;
    bool auth_completed = false;
    bool xb1_guide_pressed = false;
    uint8_t xbone_led_mode;

    bool waiting_ack = false;
    uint32_t waiting_ack_timeout = 0;
    uint32_t timer_wait_for_announce;
    bool xbox_one_powered_on;
    uint8_t report_led_mode;
    uint8_t report_led_brightness;

    XGIPProtocol *outgoingXGIP = nullptr;
    XGIPProtocol *incomingXGIP = nullptr;
    // Check report queue every 35 milliseconds
    uint32_t lastReportQueue = 0;

    std::queue<report_queue_t> report_queue;

    XboxOneDriverState xboneDriverState = NOT_READY;
};