#pragma once
#include <stdint.h>
#include "pio_spi.h"
#include "enums.pb.h"

#include "spi.hpp"

#define MODE_DIGITAL 0x41
#define MODE_ANALOG 0x73
#define MODE_ANALOG_PRESSURE 0x79
#define MODE_CONFIG 0xF3
// How long to go silent when the subtype changes, so the console's pad driver sees
// an unplug and the game re-runs its controller setup against the new subtype.
#define PSX_SWAP_DISCONNECT_MS 500
// The console counts as present if it pulled ATT low / ran a transaction this recently,
// both while listening and once active (a console that hasn't adopted the controller
// only probes the port, so the 1s DualShock watchdog alone would flap the profile).
// A PS2 only probes an empty port every 1280ms (measured), not every frame like it
// does once a pad answers, so this has to cover a couple of those probes.
#define PSX_LISTEN_TIMEOUT_MS 3000
// How long the controller keeps answering after its last user releases it. Covers a
// config reload tearing down and rebuilding the PS2 instance, which would otherwise
// look like an unplug to the console.
#define PSX_RELEASE_GRACE_MS 1000
typedef struct
{
    bool analog;
    uint8_t config[3];
} PsxReportFormat_t;
class PSXEmulation
{
public:
    PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin);
    ~PSXEmulation();
    bool ready();
    void begin(SubType type);
    // Watch ATT for the console polling the port without answering it, so the console
    // sees no controller until a profile actually calls begin() with a subtype.
    void listen();
    // Profile instances hold the controller while they exist. Once the last one lets go
    // and nothing re-acquires it within PSX_RELEASE_GRACE_MS, drop back to listen().
    void acquire();
    void release();
    void end();
    void load_state(PSXEmulation *state);
    void tick();
    void sendData(uint8_t len, uint8_t *data);
    PsxReportFormat_t getReportFormat();
    void get_rumble(uint8_t &small, uint8_t &large);
    // Stay "communicating" through a deliberate swap disconnect, otherwise the PS2
    // profile's activation trigger drops out and nothing would bring us back.
    bool is_communicating() const;

private:
    void start(SubType type, bool console_present);
    uint8_t m_users = 0;
    bool m_release_pending = false;
    uint32_t m_release_at_ms = 0;
    bool m_listening = false;
    bool m_att_seen = false;
    uint32_t m_last_att_ms = 0;
    bool m_reconnecting = false;
    SubType m_reconnect_type;
    uint32_t m_reconnect_at_ms = 0;
    volatile bool sent = true;
    pio_spi_t *spi = nullptr;
    int8_t sck;
    int8_t cmd;
    int8_t dat;
    int8_t attPin;
    int8_t ackPin;
};