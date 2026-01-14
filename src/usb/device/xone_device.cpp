#include "tusb_option.h"
// Combine together the implementation of xone and hid as they are baiscally the same
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XONE)

#define REPORT_QUEUE_INTERVAL 35
//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/xone_device.h"
#include "xgip_protocol.h"
#include <queue>
#include "usb/usb_devices.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XONE_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XONE_RX_BUFSIZE];
} xoned_interface_t;

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

// Check report queue every 35 milliseconds
static uint32_t lastReportQueue = 0;

static std::queue<report_queue_t> report_queue;

static XboxOneDriverState xboneDriverState = NOT_READY;

static uint8_t xb1_guide_on[] = {0x01, 0x5b};
static uint8_t xb1_guide_off[] = {0x00, 0x5b};

static uint8_t xboneIdle[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
                              0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Check if Auth is completed (start is 0x01, 0x01, and invalid is 0x01, 0x07)
const uint8_t authReady[] = {0x01, 0x00};

// Xbox One Announce
static uint8_t announcePacket[] = {
    0x00, 0x2a, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
    0x6f, 0x0e, 0x48, 0x02, 0x01, 0x00, 0x01, 0x00,
    0x17, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00};

uint8_t last_report[CFG_TUD_ENDPOINT0_SIZE] = {};
uint8_t last_report_counter;
uint32_t keep_alive_timer;
uint8_t keep_alive_sequence;
uint8_t virtual_keycode_sequence;
bool xb1_guide_pressed;
uint8_t xbone_led_mode;

static bool waiting_ack = false;
static uint32_t waiting_ack_timeout = 0;
static uint32_t timer_wait_for_announce;
static bool xbox_one_powered_on;
static uint8_t report_led_mode;
static uint8_t report_led_brightness;

static XGIPProtocol *outgoingXGIP = nullptr;
static XGIPProtocol *incomingXGIP = nullptr;
CFG_TUSB_MEM_SECTION static xoned_interface_t _xoned_itf[CFG_TUD_XONE];
static volatile bool sending = false;
/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num)
{
    for (uint8_t i = 0; i < CFG_TUD_XONE; i++)
    {
        if (itf_num == _xoned_itf[i].itf_num)
            return i;
    }

    return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_xone_n_ready(uint8_t itf)
{
    uint8_t const ep_in = _xoned_itf[itf].ep_in;
    return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_xone_n_report(uint8_t itf, void const *report, uint8_t len)
{
    uint8_t const rhport = 0;
    xoned_interface_t *p_xone = &_xoned_itf[itf];

    // claim endpoint
    TU_VERIFY(usbd_edpt_claim(rhport, p_xone->ep_in));

    // If report id = 0, skip ID field
    len = tu_min8(len, CFG_TUD_XONE_TX_BUFSIZE);
    memcpy(p_xone->epin_buf, report, len);
    sending = true;
    return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xone->ep_in, p_xone->epin_buf,
                          len);
}

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void xoned_init(void)
{
    xoned_reset(TUD_OPT_RHPORT);
    keep_alive_timer = to_ms_since_boot(get_absolute_time());
    keep_alive_sequence = 1; // sequence starts at 1?
    virtual_keycode_sequence = 0;
    xb1_guide_pressed = false;
    last_report_counter = 0;

    incomingXGIP = new XGIPProtocol();
    outgoingXGIP = new XGIPProtocol();

    xbone_led_mode = 0;
}

void xoned_reset(uint8_t rhport)
{
    (void)rhport;
    tu_memclr(_xoned_itf, sizeof(_xoned_itf));
    sending = false;
}
void process_report_queue(uint32_t now)
{
    if (!report_queue.empty() && (now - lastReportQueue) > REPORT_QUEUE_INTERVAL)
    {
        // if ( send_xbone_usb(report_queue.front().report, report_queue.front().len) ) {
        //     memcpy(last_report, &report_queue.front().report, report_queue.front().len);
        //     report_queue.pop();
        //     lastReportQueue = now;
        // } else {
        //     // THIS IS REQUIRED FOR TIMING ON PC / CONSOLE
        //     sleep_ms(REPORT_QUEUE_INTERVAL); // sleep while we wait, never happens during input only auth
        // }
    }
}
uint16_t xoned_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                    uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass && itf_desc->bInterfaceSubClass == 0x47 &&
                  itf_desc->bInterfaceProtocol == 0xD0,
              0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    // Find available interface
    xoned_interface_t *p_xone = NULL;
    for (uint8_t i = 0; i < CFG_TUD_XONE; i++)
    {
        if (_xoned_itf[i].ep_in == 0 && _xoned_itf[i].ep_out == 0)
        {
            p_xone = &_xoned_itf[i];
            break;
        }
    }
    TU_VERIFY(p_xone, 0);
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &p_xone->ep_out, &p_xone->ep_in), 0);

    p_xone->itf_num = itf_desc->bInterfaceNumber;

    // Prepare for output endpoint
    if (p_xone->ep_out)
    {
        if (!usbd_edpt_xfer(rhport, p_xone->ep_out, p_xone->epout_buf, sizeof(p_xone->epout_buf)))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    // Setting up XGIPs and driver state
    if (incomingXGIP != nullptr && outgoingXGIP != nullptr)
    {
        xboneDriverState = XboxOneDriverState::READY_ANNOUNCE;
        incomingXGIP->reset();
        outgoingXGIP->reset();
    }
    //------------- Endpoint Descriptor -------------//

    // Config endpoint

    return drv_len;
}

bool xoned_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    return false;
}

static void queue_xbone_report(void *report, uint16_t report_size)
{
    report_queue_t item;
    memcpy(item.report, report, report_size);
    item.len = report_size;
    report_queue.push(item);
}

bool xoned_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                   uint32_t xferred_bytes)
{
    (void)result;

    uint8_t itf = 0;
    xoned_interface_t *p_xone = _xoned_itf;

    for (;; itf++, p_xone++)
    {
        if (itf >= TU_ARRAY_SIZE(_xoned_itf))
            return false;

        if (ep_addr == p_xone->ep_out || ep_addr == p_xone->ep_in)
            break;
    }
    if (ep_addr == p_xone->ep_out)
    {
        // Parse incoming packet and verify its valid
        incomingXGIP->parse(p_xone->epout_buf, xferred_bytes);

        // Setup an ack before we change anything about the incoming packet
        if (incomingXGIP->ackRequired() == true)
        {
            queue_xbone_report((uint8_t *)incomingXGIP->generateAckPacket(), incomingXGIP->getPacketLength());
        }

        // uint8_t command = incomingXGIP->getCommand();
        // if (command == GIP_ACK_RESPONSE)
        // {
        //     waiting_ack = false;
        // }
        // else if (command == GIP_DEVICE_DESCRIPTOR)
        // {
        //     // setup descriptor packet
        //     outgoingXGIP->reset(); // reset if anything was in there
        //     outgoingXGIP->setAttributes(GIP_DEVICE_DESCRIPTOR, incomingXGIP->getSequence(), 1, 1, 0);
        //     outgoingXGIP->setData(xboxOneDescriptor, sizeof(xboxOneDescriptor));
        //     xboneDriverState = XboxOneDriverState::SEND_DESCRIPTOR;
        // }
        // else if (command == GIP_POWER_MODE_DEVICE_CONFIG)
        // {
        //     // Power Mode On!
        //     xbox_one_powered_on = true;
        // }
        // else if (command == GIP_CMD_LED_ON)
        // {
        //     // Set all player LEDs to on
        //     report_led_mode = incomingXGIP->getData()[1];       // 1 - turn LEDs on
        //     report_led_brightness = incomingXGIP->getData()[2]; // 2 - brightness (ignored for now)

        //     // If a controller has been seen before, the protocol skips to reading the device descriptor
        //     if (xboneDriverState == XboxOneDriverState::WAIT_DESCRIPTOR_REQUEST)
        //     {
        //         outgoingXGIP->reset(); // reset if anything was in there
        //         outgoingXGIP->setAttributes(GIP_DEVICE_DESCRIPTOR, incomingXGIP->getSequence(), 1, 1, 0);
        //         outgoingXGIP->setData(xboxOneDescriptor, sizeof(xboxOneDescriptor));
        //         xboneDriverState = XboxOneDriverState::SEND_DESCRIPTOR;
        //     }
        // }
        // else if (command == GIP_CMD_RUMBLE)
        // {
        //     // TO-DO
        // }
        // else if (command == GIP_AUTH || command == GIP_FINAL_AUTH)
        // {
        //     if (incomingXGIP->getDataLength() == 2 && memcmp(incomingXGIP->getData(), authReady, sizeof(authReady)) == 0)
        //     {
        //         xboxOneAuthData->authCompleted = true;
        //         xboneDriverState = AUTH_DONE;
        //     }
        //     if ((incomingXGIP->getChunked() == true && incomingXGIP->endOfChunk() == true) ||
        //         (incomingXGIP->getChunked() == false))
        //     {
        //         xboxOneAuthData->consoleBuffer.setBuffer(incomingXGIP->getData(), incomingXGIP->getDataLength(),
        //                                                  incomingXGIP->getSequence(), incomingXGIP->getCommand());
        //         xboxOneAuthData->xboneState = GPAuthState::send_auth_console_to_dongle;
        //         incomingXGIP->reset();
        //     }
        // }

        TU_ASSERT(usbd_edpt_xfer(rhport, p_xone->ep_out, p_xone->epout_buf,
                                 sizeof(p_xone->epout_buf)));
    }
    else if (ep_addr == p_xone->ep_in)
    {
        // Nothing needed
        sending = false;
    }
    return true;
}

#endif

XboxOneGamepadDevice::XboxOneGamepadDevice()
{
}
void XboxOneGamepadDevice::initialize()
{
}
void XboxOneGamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || !m_eps_assigned || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    // TODO: this
}

size_t XboxOneGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : m_interface,
        Reserved : 0x01,
        CompatibleID : "XUSB10",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    assert(sizeof(section) <= remaining);
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t XboxOneGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epin();
    }
    uint8_t desc[] = {TUD_XONE_GAMEPAD_DESCRIPTOR(m_interface, m_epin, m_epout)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void XboxOneGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = XBOX_ONE_CONTROLLER_VID;
    desc->idProduct = XBOX_ONE_CONTROLLER_PID;
    desc->bDeviceClass = 0xff;
    desc->bDeviceSubClass = 0x47;
    desc->bDeviceProtocol = 0xd0;
}