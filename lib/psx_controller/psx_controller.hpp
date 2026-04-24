#pragma once
#include <stdint.h>

#include "spi.hpp"
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "pico/time.h"
#include <hardware/gpio.h>

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
#define INTER_CMD_BYTE_DELAY 100
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
#define ATTN_DELAY 100

typedef enum 
{
    DISCONNECTED,
    CONNECTION_DELAY,
    FIRST_INPUTS,
    ENTER_CONFIG,
    ENABLE_ANALOG_MODE,
    ENABLE_PRESSURES,
    ENABLE_PRESSURES_2,
    EXIT_CONFIG,
    SECOND_INPUTS,
    ENUMERATED
} PSXControllerState;

class PSXController
{
public:
    PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin);
    void tick();
    PS2ControllerType type = PS2ControllerTypeUnknown;
    uint16_t readAxis(PS2AxisType type);
    bool readButton(PS2ButtonType type);
    bool controller_valid();
    void processData(bool ack, bool timeout);

private:
    bool autoShiftData(const uint8_t *out, const uint8_t len);
    void noAttention();
    void signalAttention();
    SPIMasterInterface interface;
    uint8_t m_attPin;
    uint8_t m_ackPin;
    int missing = 0;
    bool valid = false;
    bool hasTapBar = false;
    long last = 0;
    long lastInit = 0;
    uint8_t invalidCount = 0;
    uint8_t ps2Data[BUFFER_SIZE];
    uint8_t lastInputs[BUFFER_SIZE];
    const uint8_t* ps2DataOut;
    uint8_t ps2Idx;
    uint8_t ps2Len;
    uint8_t ps2DataLen;
    alarm_id_t timeout_alarm_id;
    bool done = false;
    uint32_t lastScan = 0;
    PSXControllerState status = DISCONNECTED;
    uint32_t packet_delay = 10000;
};