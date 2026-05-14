#pragma once
#include "host.hpp"
#include "protocols/xbox_one.hpp"
#include <queue>

typedef enum
{
    WAIT,
    WAKEUP,
    LED_ON
} XboxOneHostState;
typedef struct
{
    uint8_t report[CFG_TUD_XONE_TX_BUFSIZE];
    uint16_t len;
} report_queue_t;
class XboxOneHost : public UsbHostInterface
{
public:
    ~XboxOneHost() {}
    XboxOneHost(uint8_t dev_addr, uint8_t interface, uint16_t id);
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);
    void update(bool full_poll, bool send_events);

private:
    void queue_xbone_report(void *report, uint16_t report_size);
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    CFG_TUSB_MEM_ALIGN uint8_t m_last_inputs[64];
    XGIPProtocol *outgoingXGIP = nullptr;
    XGIPProtocol *incomingXGIP = nullptr;

    std::queue<report_queue_t> report_queue;
};