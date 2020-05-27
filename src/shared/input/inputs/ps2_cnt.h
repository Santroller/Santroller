#pragma once
#ifdef __AVR__
#  include "controller/controller.h"
#  include <stdbool.h>
#  include <stdint.h>
/** \brief Command Inter-Byte Delay (us)
 *
 * Commands are several bytes long. This is the time to wait between two
 * consecutive bytes.
 *
 * This should actually be done by watching the \a Acknowledge line, but we are
 * ignoring it at the moment.
 */
#  define INTER_CMD_BYTE_DELAY 15

/** \brief Command timeout (ms)
 *
 * Commands are sent to the controller repeatedly, until they succeed or time
 * out. This is the length of that timeout.
 *
 * \sa COMMAND_RETRY_INTERVAL
 */
#  define COMMAND_TIMEOUT 250

/** \brief Command Retry Interval (ms)
 *
 * When sending a command to the controller, if it does not succeed, it is
 * retried after this amount of time.
 */
#  define COMMAND_RETRY_INTERVAL 10

/** \brief Attention Delay
 *
 * Time between attention being issued to the controller and the first clock
 * edge (us).
 */
#  define ATTN_DELAY 15
enum PsxButton {
  PSB_SELECT,
  PSB_L3,
  PSB_R3,
  PSB_START,
  PSB_PAD_UP,
  PSB_PAD_RIGHT,
  PSB_PAD_DOWN,
  PSB_PAD_LEFT,
  PSB_L2,
  PSB_R2,
  PSB_L1,
  PSB_R1,
  PSB_TRIANGLE,
  PSB_CIRCLE,
  PSB_CROSS,
  PSB_SQUARE
};
enum FightStickButton {
  FSB_START=3,
  FSB_PAD_UP,
  FSB_PAD_RIGHT,
  FSB_PAD_DOWN,
  FSB_PAD_LEFT,
  FSB_L2,
  FSB_L1,
  FSB_SQUARE,
  FSB_TRIANGLE,
  FSB_R1,
  FSB_CIRCLE,
  FSB_CROSS,
  FSB_R2,
};
enum NegconButton {
  NGB_START=3,
  NGB_PAD_UP,
  NGB_PAD_RIGHT,
  NGB_PAD_DOWN,
  NGB_PAD_LEFT,
  NGB_R1 = 11,
  NGB_A,
  NGB_B,
};
enum MouseButton {
  PMB_LEFT=9,
  PMB_RIGHT,
};

enum GHButton {
  GH_STAR_POWER = PSB_L2,
  GH_GREEN = PSB_R2,
  GH_RED = PSB_CIRCLE,
  GH_YELLOW = PSB_TRIANGLE,
  GH_BLUE = PSB_CROSS,
  GH_ORANGE = PSB_SQUARE
};

enum PsxAnalogButton {
  PSAB_PAD_RIGHT,
  PSAB_PAD_LEFT,
  PSAB_PAD_UP,
  PSAB_PAD_DOWN,
  PSAB_TRIANGLE,
  PSAB_CIRCLE,
  PSAB_CROSS,
  PSAB_SQUARE,
  PSAB_L1,
  PSAB_R1,
  PSAB_L2,
  PSAB_R2
};
enum GHAnalogButton { GH_WHAMMY = PSAB_L1 };

void initPS2CtrlInput(void);
void tickPS2CtrlInput(Controller_t *controller);
extern uint8_t ps2CtrlType;
#endif

enum PsxControllerType {
  PSX_UNKNOWN_CONTROLLER = 0,
  PSX_DUALSHOCK_1_CONTROLLER,
  PSX_DUALSHOCK_2_CONTROLLER,
  PSX_WIRELESS_SONY_DUALSHOCK_CONTROLLER,
  PSX_GUITAR_HERO_CONTROLLER,
  PSX_NEGCON,
  PSX_ANALOG,
  PSX_MOUSE,
  PSX_NO_DEVICE
};