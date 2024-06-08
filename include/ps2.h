#include <stdint.h>
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif

/** \brief Size of internal communication buffer
 *
 * This can be sized after the longest command reply (which is 21 bytes for
 * 01 42 when in DualShock 2 mode), but we're better safe than sorry.
 */
#define BUFFER_SIZE 32
/** \brief Command Inter-Byte Delay (us)
 *
 * Commands are several bytes long. This is the time to wait between two
 * consecutive bytes.
 *
 * This should actually be done by watching the \a Acknowledge line, but we are
 * ignoring it at the moment.
 */
#define INTER_CMD_BYTE_DELAY 50
/** \brief Command timeout (ms)
 *
 * Commands are sent to the controller repeatedly, until they succeed or time
 * out. This is the length of that timeout.
 *
 * \sa COMMAND_RETRY_INTERVAL
 */
#define COMMAND_TIMEOUT 250

/** \brief Command Retry Interval (ms)
 *
 * When sending a command to the controller, if it does not succeed, it is
 * retried after this amount of time.
 */
#define COMMAND_RETRY_INTERVAL 10

/** \brief Attention Delay
 *
 * Time between attention being issued to the controller and the first clock
 * edge (us).
 */
#define ATTN_DELAY 50
enum PsxButton {
    // PSB_NONE,
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
enum MultitapPort { A = 0x01,
                    B = 0x02,
                    C = 0x03,
                    D = 0x04 };
extern uint8_t ps2ControllerType;
uint8_t* tickPS2(void);
bool ps2_emu_tick(PS2_REPORT* report);
void ps2_emu_init(void);
#ifdef __cplusplus
}
#endif