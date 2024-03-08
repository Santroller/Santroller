#include "config.h"


// State of the Xbox One controller initilisation
extern Xbox_One_State_t xbox_one_state;

// State of the Xbox 360 controller init
extern Xbox_360_State_t xbox_360_state;
extern uint16_t xbox_360_vid;
extern uint16_t xbox_360_pid;

// Used for passing authentication data between the console and authentication controller
extern uint8_t data_from_console_size;
extern uint8_t data_from_controller_size;
extern uint8_t data_from_console[64];
extern uint8_t data_from_controller[64];

// Sequence numbers used for sending reports to the xbox one
extern uint8_t report_sequence_number;
extern uint8_t keystroke_sequence_number;
extern uint8_t hid_sequence_number;

// Sequence number used for sending reports to the ps4
extern uint8_t ps4_sequence_number;

// Used for ps4 auth
extern bool auth_ps4_controller_found;

// Used for differenciating ps3 and ps4
extern bool seen_ps4;

// Used for detecting wii
extern bool seen_hid_descriptor_read;

// Used for detecting pademu
extern bool descriptor_requested;

// Use for configuring commands that are pc specific
extern bool seen_windows_xb1;
extern bool seen_windows;
extern bool read_any_string;

// Used for detecting any connection at all
extern bool read_device_desc;
extern bool lastEuphoriaLed;