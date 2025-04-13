#pragma once
#include "interfaces/gpio.hpp"
#include "interfaces/spi.hpp"
#include "state/base.hpp"
#include "parsers/ps2.hpp"

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

enum MultitapPort { A = 0x01,
                    B = 0x02,
                    C = 0x03,
                    D = 0x04 };

class PS2Device {
   public:
    PS2Device(SPIMasterInterface* interface, GPIOInterface* csPin, GPIOInterface* attPin);
    void tick(san_base_t* data);

   private:
    bool autoShiftData(uint8_t port, uint8_t* in, const uint8_t* out, const uint8_t len);
    void shiftDataInOut(const uint8_t* out, uint8_t* in, const uint8_t len);
    bool sendCommand(uint8_t port, uint8_t* in, const uint8_t* buf, uint8_t len);
    void noAttention();
    void signalAttention();
    SPIMasterInterface* mInterface;
    GPIOInterface* mCsPin;
    GPIOInterface* mAttPin;
    PS2Parser mParser;
    bool mFound;
    long last = 0;
    uint8_t invalidCount = 0;
};