#include "controllers.h"
#include "endpoints.h"
#ifdef __cplusplus
extern "C" {
#endif
bool seen_hid_descriptor_read = false;
bool seen_ps4 = false;
bool seen_rpcs3 = false;
bool descriptor_requested = false;
Xbox_One_State_t xbox_one_state = Announce;
Xbox_360_State_t xbox_360_state = Auth1;
uint8_t data_from_console_size = 0;
uint8_t data_from_controller_size = 0;
uint8_t data_from_console[64];
uint8_t data_from_controller[64];
uint8_t keystroke_sequence_number;
uint8_t report_sequence_number;
uint8_t hid_sequence_number;
uint8_t ps4_sequence_number;
uint8_t portal_state = 0;
bool seen_windows_xb1 = 0;
bool seen_os_descriptor_read = 0;
bool seen_og_xbox = false;
bool seen_windows = false;
bool read_any_device_string = false;
bool read_device_desc = false;
bool lastEuphoriaLed = false;
bool proButtonsEnabled = false;
#ifdef __cplusplus
}
#endif