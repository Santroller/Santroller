#include "motion.h"
#include "protocols/wiimote_calibration.h"

// Nothing moves the pointer, so the remote always points at the middle of the screen, held level. This
// is what working that out from a 1m wide 4:3 screen 2m away, with a 20cm sensor bar on its top edge and
// the camera's 42 degree field of view gives: the sensor bar's two dots just below the middle of the
// camera's 1024 x 768 view, as small as they get, and gravity straight down the remote's Z.
#define SENSOR_BAR_LEFT_X 462
#define SENSOR_BAR_RIGHT_X 561
#define SENSOR_BAR_Y 571
#define SENSOR_BAR_SIZE 1

void set_motion_state(struct wiimote_state *state)
{
  reset_ir_object(&state->usr.ir_object[0]);
  reset_ir_object(&state->usr.ir_object[1]);
  state->usr.ir_object[0].x = SENSOR_BAR_LEFT_X;
  state->usr.ir_object[0].y = SENSOR_BAR_Y;
  state->usr.ir_object[0].size = SENSOR_BAR_SIZE;
  state->usr.ir_object[1].x = SENSOR_BAR_RIGHT_X;
  state->usr.ir_object[1].y = SENSOR_BAR_Y;
  state->usr.ir_object[1].size = SENSOR_BAR_SIZE;

  state->usr.accel_x = WIIMOTE_EMULATED_ACCEL_ZERO;
  state->usr.accel_y = WIIMOTE_EMULATED_ACCEL_ZERO;
  state->usr.accel_z = WIIMOTE_EMULATED_ACCEL_ONE_G;
}
