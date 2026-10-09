#include <stdint.h>
extern uint8_t const desc_hid_report_buttons[194];
extern uint8_t const desc_hid_report_hat[199];
extern uint8_t const desc_hid_report_keyboard[];
extern const uint16_t desc_hid_report_keyboard_len;
// The keyboard / mouse GATT database (bt_keyboard.gatt), whose HID service only references the reports in
// desc_hid_report_keyboard. Returns the database, and the handle range of its config HID service.
const uint8_t *bt_keyboard_profile(uint16_t *config_start, uint16_t *config_end);
