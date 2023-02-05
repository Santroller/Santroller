#include "controllers.h"
#ifdef __cplusplus
extern "C" {
#endif
bool read_hid_report_descriptor = false;
bool set_idle = false;
bool windows_or_xbox_one = false;
Xbox_One_State_t xbox_one_state = 0;
uint8_t fromConsoleLen = 0;
uint8_t fromControllerLen = 0;
uint8_t fromConsole[64];
uint8_t fromController[64];
uint8_t arrivalSequenceNumber;
uint8_t reportSequenceNumber;
uint8_t hidSequenceNumber;
#ifdef __cplusplus
}
#endif