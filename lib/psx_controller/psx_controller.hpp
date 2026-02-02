#pragma once
#include <stdint.h>

#include "spi.hpp"
#include "enums.pb.h"
#include "input_enums.pb.h"

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

typedef enum
{
    BASE = 0x00,
    A = 0x01,
    B = 0x02,
    C = 0x03,
    D = 0x04
} MultitapPort;
class PSXController
{
public:
    PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin, MultitapPort port);
    void tick();
    inline bool is_connected()
    {
        return connected;
    }
    PS2ControllerType type = PS2ControllerTypeUnknown;
    uint16_t readAxis(PS2AxisType type);
    bool readButton(PS2ButtonType type);
    bool controller_valid(MultitapPort port);

private:
    bool autoShiftData(uint8_t port, uint8_t *in, const uint8_t *out, const uint8_t len);
    void shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len);
    bool sendCommand(uint8_t port, uint8_t *in, const uint8_t *buf, uint8_t len);
    void noAttention();
    void signalAttention();
    SPIMasterInterface interface;
    MultitapPort m_port;
    uint8_t attPin;
    uint8_t ackPin;
    int missing;
    bool connected;
    bool hasTapBar = false;
    long last = 0;
    uint8_t invalidCount = 0;
    uint8_t ps2Data[BUFFER_SIZE];
};