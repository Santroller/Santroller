#include "controllers.h"
#include "endpoints.h"
#ifdef __cplusplus
extern "C" {
#endif
long xbox_timer = 0;
long ps5_timer = 0;
long wii_timer = 0;
bool read_config = false;
bool received_after_read_config = false;
bool read_hid_report_descriptor = false;
long last_ghl_poke_time = 0;
bool set_idle = false;
bool windows_or_xbox_one = false;
Xbox_One_State_t xbox_one_state = Announce;
uint16_t xbox_360_vid = ARDWIINO_VID;
uint16_t xbox_360_pid = ARDWIINO_360_PID;
bool xbox_360_authenticated = false;
uint8_t fromConsoleLen = 0;
uint8_t fromControllerLen = 0;
uint8_t fromConsole[64];
uint8_t fromController[64];
uint8_t reportSequenceNumber;
uint8_t hidSequenceNumber;
#ifdef __cplusplus
}
#endif