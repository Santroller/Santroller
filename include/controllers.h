#include "config.h"

// Timer used for detection if a GHL Xbox one controller is in GHL mode or navigation mode
extern long last_ghl_poke_time;

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

// If a stage kit is used for auth, pass any stage kit commands through to it
extern bool passthrough_stage_kit;

// If we successfully managed to communicate with a RF module, flag that
extern bool rf_initialised;
extern bool rf_connected;

// Used for ps4 auth
extern bool auth_ps4_controller_found;

// Used for detecting wii
extern bool seen_non_wii_packet;

// Use for configuring commands that are pc specific
extern bool windows;