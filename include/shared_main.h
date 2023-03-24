#include "pins.h"
#include "reports/controller_reports.h"

void init_main(void);

void device_reset(void);
void tick(void);
uint8_t tick_inputs(uint8_t* buf);
void reset_usb(void);
uint8_t transfer_with_usb_controller(const uint8_t device, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t* buffer);
void send_report_to_controller(uint8_t *report, uint8_t len);
void send_report_to_pc(const void *report, uint8_t len);
bool ready_for_next_packet(void);
bool usb_connected(void);
void receive_report_from_controller(uint8_t const *report, uint16_t len);
void xinput_controller_connected(uint8_t vid, uint8_t pid, uint8_t subtype);
void xone_controller_connected(void);
void controller_disconnected(void);
void send_rf_console_type(void);
extern PS3_REPORT last_bt_report;