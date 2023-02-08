#include "config.h"

// Different state during usb initialisation that are used to detect different consoles
extern bool read_hid_report_descriptor;
extern bool set_idle;
extern bool windows_or_xbox_one;
extern bool read_config;
extern bool received_after_read_config;

// Timer used for detection if a GHL Xbox one controller is in GHL mode or navigation mode
extern long last_ghl_poke_time;

// Timers used for console detection
extern long xbox_timer;
extern long ps5_timer;
extern long wii_timer;

// State of the Xbox One controller initilisation
extern Xbox_One_State_t xbox_one_state;

// State of the Xbox 360 controller init
extern Xbox_360_State_t xbox_360_state;
extern uint16_t xbox_360_vid;
extern uint16_t xbox_360_pid;

// Used for passing authentication data between the console and authentication controller
extern uint8_t fromConsoleLen;
extern uint8_t fromControllerLen;
extern uint8_t fromConsole[64];
extern uint8_t fromController[64];

// Sequence numbers used for sending reports to the xbox one
extern uint8_t reportSequenceNumber;
extern uint8_t keystrokeSequenceNumber;
extern uint8_t hidSequenceNumber;