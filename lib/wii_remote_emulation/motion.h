#ifndef MOTION_H
#define MOTION_H

#include "wiimote.h"

// The IR camera and accelerometer of a remote pointing at the middle of the screen
void set_motion_state(struct wiimote_state *state);

#endif