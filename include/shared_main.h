#include "pins.h"
#include "reports/controller_reports.h"

void init_main(void);

void device_reset(void);
void tick(void);
void reset_usb(void);
void send_report_to_controller(uint8_t *report, uint8_t len);
void send_report_to_pc(const void *report, uint8_t len);
bool ready_for_next_packet(void);
bool usb_connected(void);
void received_any_request(void);
void receive_report_from_controller(uint8_t const *report, uint16_t len);
void xinput_controller_connected(uint8_t vid, uint8_t pid);
void xone_controller_connected(void);
void controller_disconnected(void);