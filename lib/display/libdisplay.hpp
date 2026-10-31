#pragma once

#include "i2c.hpp"
#include "spi.hpp"
#include "pico/time.h"
#include "mono_framebuffer.hpp"

// The frame goes out this many columns at a time, so the bus is never held for long by one transfer
#define MONO_DISPLAY_CHUNK 32
#define MONO_DISPLAY_MAX_CHUNKS (MonoFramebuffer::MAX_WIDTH / MONO_DISPLAY_CHUNK)

typedef enum
{
    MONO_DISPLAY_SSD1306,
    MONO_DISPLAY_SSD1309,
    MONO_DISPLAY_SH1106,
    MONO_DISPLAY_SH1107,
} mono_display_controller_e;

// Where the display is: I2C when spi is false, otherwise SPI. Pins that aren't connected are -1, cs and
// reset can be wired to GND and 3V3 instead.
typedef struct
{
    bool spi;
    uint8_t block;
    int8_t sda;
    int8_t scl;
    uint32_t clock;
    uint8_t address;
    int8_t sck;
    int8_t mosi;
    int8_t dc;
    int8_t cs;
    int8_t reset;
} mono_display_bus_t;

typedef enum
{
    MONO_DISPLAY_IDLE,
    MONO_DISPLAY_INIT,
    MONO_DISPLAY_CHUNK_WRITE,
    MONO_DISPLAY_POWER,
} mono_display_status_e;

// An SSD1306, SSD1309, SH1106 or SH1107 OLED on I2C or SPI. Everything goes out from tick(), only the
// parts of the frame that changed, and the I2C interrupt or SPI DMA just notes when each transfer is done.
class MonoDisplay : public I2CDMAInterface
{
public:
    // width only matters for the SH1107 and height for the rest, rotation is in quarter turns clockwise
    MonoDisplay(const mono_display_bus_t &bus, mono_display_controller_e controller, uint8_t width,
                uint8_t height, uint8_t rotation, uint8_t contrast);
    void tick();
    void begin();
    void end();
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return m_connected;
    }
    // What to show, drawn into from the main loop. Call frame_changed() after drawing, then only the parts
    // that differ from what the display has are sent.
    MonoFramebuffer &frame() { return m_frame; }
    void frame_changed() { m_check = true; }
    // Whether the display has everything that has been drawn, so drawing again now won't be wasted
    bool caught_up() const { return m_connected && !m_check && !m_busy; }
    // Turns the panel off, or back on, keeping what it shows
    void set_on(bool on);

private:
    // Sends commands, followed by m_chunk when data is set
    bool send(mono_display_status_e status, const uint8_t *commands, uint8_t len, bool data);
    void spi_done();
    bool next_chunk(uint8_t &page, uint8_t &chunk);
    uint8_t pages() const { return m_frame.panel_height() / 8; }
    uint8_t chunks() const { return m_frame.panel_width() / MONO_DISPLAY_CHUNK; }
    // Only the one the display is on has pins
    I2CMasterInterface interface;
    SPIMasterInterface spi_interface;
    bool m_spi;
    int8_t m_dc;
    int8_t m_cs;
    int8_t m_reset;
    uint8_t m_address;
    mono_display_controller_e m_controller;
    uint8_t m_contrast;
    MonoFramebuffer m_frame;
    // What the display has (only its pages are used), once m_unknown has been cleared for that chunk
    MonoFramebuffer m_shown;
    // One bit per chunk (page * MONO_DISPLAY_MAX_CHUNKS + chunk) whose contents on the display aren't known
    uint64_t m_unknown = 0;
    // Set when the frame might differ from the display, cleared once a check finds nothing to send, so
    // nothing is compared while it's idle
    bool m_check = false;
    uint8_t m_scan_from = 0;
    bool m_on = true;
    bool m_panel_on = false;
    bool m_connected = false;
    mono_display_status_e m_status = MONO_DISPLAY_IDLE;
    volatile bool m_busy = false;
    volatile bool m_failed = false;
    uint8_t m_sending_page = 0;
    uint8_t m_sending_chunk = 0;
    bool m_sending_on = false;
    uint32_t m_sent_at = 0;
    uint32_t m_retry_at = 0;
    int m_fail_count = 0;
    bool m_stopped = true;
    // The chunk being sent, which is what the display has once it's done
    uint8_t m_chunk[MONO_DISPLAY_CHUNK];
    uint8_t m_tx[48];
    uint16_t m_cmds[48];
};
