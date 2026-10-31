#include "libdisplay.hpp"
#include <string.h>
#include "utils.h"
#include <hardware/gpio.h>

// Control bytes: the rest of the transfer is commands, one command then another control byte, or the
// rest of the transfer is data
#define CONTROL_COMMANDS 0x00
#define CONTROL_ONE_COMMAND 0x80
#define CONTROL_DATA 0x40
// How long to wait before trying again when it isn't answering
#define MONO_DISPLAY_RETRY_MS 500
// The SH1106 has 132 columns, the 128 that are visible start at 2
#define SH1106_COLUMN_OFFSET 2
// The fastest SPI clocks they're rated for
#define SSD130X_SPI_CLOCK 8000000
#define SH110X_SPI_CLOCK 4000000

// Panel off, clock, display offset and start line, the controller's own power setup, then follow RAM
// and not inverted. The settings that change go after these.
static const uint8_t ssd1306_init[] = {
    0xAE, 0xD5, 0x80, 0xD3, 0x00, 0x40,
    // charge pump on, page addressing like the SH1106, no scrolling
    0x8D, 0x14, 0x20, 0x02, 0x2E,
    // precharge, VCOMH
    0xD9, 0xF1, 0xDB, 0x40,
    0xA4, 0xA6};
// The same, but it has no charge pump, the panel's supply comes from outside
static const uint8_t ssd1309_init[] = {
    0xAE, 0xD5, 0x80, 0xD3, 0x00, 0x40,
    0x20, 0x02, 0x2E,
    0xD9, 0xF1, 0xDB, 0x40,
    0xA4, 0xA6};
static const uint8_t sh1106_init[] = {
    0xAE, 0xD5, 0x80, 0xD3, 0x00, 0x40,
    // DC-DC converter on, pump voltage 8V
    0xAD, 0x8B, 0x32,
    // precharge, VCOMH
    0xD9, 0x22, 0xDB, 0x35,
    0xA4, 0xA6};
// Panel off, clock, page addressing, DC-DC converter on, start line 0, precharge, VCOMH, follow RAM and
// not inverted. Its start line command takes the line as a second byte.
static const uint8_t sh1107_init[] = {
    0xAE, 0xD5, 0x51, 0x20, 0xAD, 0x8A, 0xDC, 0x00,
    0xD9, 0x22, 0xDB, 0x35,
    0xA4, 0xA6};

static uint32_t spi_clock(mono_display_controller_e controller)
{
    return controller == MONO_DISPLAY_SH1106 || controller == MONO_DISPLAY_SH1107 ? SH110X_SPI_CLOCK
                                                                                   : SSD130X_SPI_CLOCK;
}

// Only the bus it's on is given pins, the other is left alone
MonoDisplay::MonoDisplay(const mono_display_bus_t &bus, mono_display_controller_e controller, uint8_t width,
                         uint8_t height, uint8_t rotation, uint8_t contrast)
    : interface(bus.block, bus.spi ? -1 : bus.sda, bus.spi ? -1 : bus.scl, bus.clock),
      spi_interface(bus.block, SPI_CPHA_0, SPI_CPOL_0, bus.spi ? bus.sck : -1, bus.spi ? bus.mosi : -1, -1, true,
                    spi_clock(controller)),
      m_spi(bus.spi),
      // Without the pin that picks commands or data, nothing can be sent
      m_dc(bus.spi && bus.sck >= 0 && bus.mosi >= 0 ? bus.dc : -1), m_cs(bus.spi ? bus.cs : -1),
      m_reset(bus.spi ? bus.reset : -1), m_address(bus.address), m_controller(controller), m_contrast(contrast)
{
    // The SH1107 is 64 or 128 wide and 128 tall, the rest are 128 wide and 64 or 32 tall
    if (controller == MONO_DISPLAY_SH1107)
    {
        m_frame.configure(width == 64 ? 64 : 128, 128, rotation);
    }
    else
    {
        m_frame.configure(128, height == 32 ? 32 : 64, rotation);
    }
}

void MonoDisplay::begin()
{
    m_stopped = false;
    m_connected = false;
    m_busy = false;
    m_status = MONO_DISPLAY_IDLE;
    m_fail_count = 0;
    m_retry_at = 0;
    if (!m_spi)
    {
        interface.dmaInit(m_address, this);
        return;
    }
    if (m_dc < 0)
    {
        return;
    }
    gpio_init(m_dc);
    gpio_set_dir(m_dc, GPIO_OUT);
    if (m_cs >= 0)
    {
        gpio_init(m_cs);
        gpio_put(m_cs, 1);
        gpio_set_dir(m_cs, GPIO_OUT);
    }
    if (m_reset >= 0)
    {
        // They want reset held low for at least 10us, then a moment before the first command
        gpio_init(m_reset);
        gpio_set_dir(m_reset, GPIO_OUT);
        gpio_put(m_reset, 0);
        busy_wait_us(20);
        gpio_put(m_reset, 1);
        m_retry_at = millis() + 2;
    }
}

void MonoDisplay::end()
{
    m_stopped = true;
    if (m_spi)
    {
        spi_interface.transfer_dma_abort();
        spi_done();
    }
    interface.dmaDeinit(m_address);
    m_connected = false;
    m_busy = false;
    m_status = MONO_DISPLAY_IDLE;
}

void MonoDisplay::set_on(bool on)
{
    m_on = on;
}

void MonoDisplay::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    m_failed = timeout || abort_detected;
    m_busy = false;
}

void MonoDisplay::spi_done()
{
    if (m_cs >= 0)
    {
        gpio_put(m_cs, 1);
    }
    m_busy = false;
}

bool MonoDisplay::send(mono_display_status_e status, const uint8_t *commands, uint8_t len, bool data)
{
    m_status = status;
    m_failed = false;
    m_busy = true;
    m_sent_at = millis();
    if (m_spi)
    {
        // Nothing comes back on SPI, so a display that's there is just assumed. The commands are only a
        // few bytes, the data goes out by DMA while the main loop carries on.
        if (m_dc < 0)
        {
            m_busy = false;
            m_failed = true;
            return false;
        }
        if (m_cs >= 0)
        {
            gpio_put(m_cs, 0);
        }
        gpio_put(m_dc, 0);
        spi_interface.transfer(commands, nullptr, len);
        if (data)
        {
            gpio_put(m_dc, 1);
            if (spi_interface.transfer_dma_start(m_chunk, nullptr, MONO_DISPLAY_CHUNK))
            {
                return true;
            }
            spi_interface.transfer(m_chunk, nullptr, MONO_DISPLAY_CHUNK);
        }
        spi_done();
        return true;
    }
    // On I2C a control byte says whether commands or data follow: all commands, or one command at a time
    // until the data
    uint8_t n = 0;
    if (data)
    {
        for (uint8_t i = 0; i < len; i++)
        {
            m_tx[n++] = CONTROL_ONE_COMMAND;
            m_tx[n++] = commands[i];
        }
        m_tx[n++] = CONTROL_DATA;
        memcpy(&m_tx[n], m_chunk, MONO_DISPLAY_CHUNK);
        n += MONO_DISPLAY_CHUNK;
    }
    else
    {
        m_tx[n++] = CONTROL_COMMANDS;
        memcpy(&m_tx[n], commands, len);
        n += len;
    }
    if (!interface.dmaWriteRead(m_address, m_tx, n, nullptr, 0, m_cmds))
    {
        m_busy = false;
        m_failed = true;
        return false;
    }
    return true;
}

bool MonoDisplay::next_chunk(uint8_t &page, uint8_t &chunk)
{
    // Carries on from the last chunk found, so sending a frame compares each chunk about once rather than
    // starting over after every one
    uint8_t total = pages() * chunks();
    for (uint8_t i = 0; i < total; i++)
    {
        uint8_t idx = (m_scan_from + i) % total;
        page = idx / chunks();
        chunk = idx % chunks();
        uint64_t bit = 1ull << (page * MONO_DISPLAY_MAX_CHUNKS + chunk);
        uint8_t col = chunk * MONO_DISPLAY_CHUNK;
        if ((m_unknown & bit) || memcmp(&m_frame.pages[page][col], &m_shown.pages[page][col], MONO_DISPLAY_CHUNK))
        {
            m_scan_from = idx;
            return true;
        }
    }
    return false;
}

void MonoDisplay::tick()
{
    interface.tick();
    if (m_stopped)
    {
        return;
    }
    uint32_t now = millis();
    if (m_spi && m_busy && !spi_interface.transfer_dma_busy())
    {
        spi_done();
    }
    if (m_busy)
    {
        if (now - m_sent_at < 1000)
        {
            return;
        }
        // The transfer never came back
        if (m_spi)
        {
            spi_interface.transfer_dma_abort();
            spi_done();
        }
        m_busy = false;
        m_failed = true;
    }
    if (m_status != MONO_DISPLAY_IDLE)
    {
        mono_display_status_e status = m_status;
        m_status = MONO_DISPLAY_IDLE;
        if (m_failed)
        {
            // Ride out the occasional NAK, whatever failed is still different so it goes again. Only drop
            // it once it stops answering.
            if (!m_connected || ++m_fail_count > 10)
            {
                m_connected = false;
                m_fail_count = 0;
                m_retry_at = now + MONO_DISPLAY_RETRY_MS;
            }
            return;
        }
        m_fail_count = 0;
        switch (status)
        {
        case MONO_DISPLAY_INIT:
            m_connected = true;
            m_panel_on = false;
            m_unknown = UINT64_MAX;
            m_check = true;
            break;
        case MONO_DISPLAY_CHUNK_WRITE:
            memcpy(&m_shown.pages[m_sending_page][m_sending_chunk * MONO_DISPLAY_CHUNK], m_chunk, MONO_DISPLAY_CHUNK);
            m_unknown &= ~(1ull << (m_sending_page * MONO_DISPLAY_MAX_CHUNKS + m_sending_chunk));
            break;
        case MONO_DISPLAY_POWER:
            m_panel_on = m_sending_on;
            break;
        default:
            break;
        }
    }
    if (!m_connected)
    {
        if ((int32_t)(now - m_retry_at) < 0)
        {
            return;
        }
        // Starts with the panel off, it's turned on once it has a whole frame so whatever was in its RAM
        // is never shown
        const uint8_t *init = ssd1306_init;
        uint8_t len = sizeof(ssd1306_init);
        switch (m_controller)
        {
        case MONO_DISPLAY_SSD1309:
            init = ssd1309_init;
            len = sizeof(ssd1309_init);
            break;
        case MONO_DISPLAY_SH1106:
            init = sh1106_init;
            len = sizeof(sh1106_init);
            break;
        case MONO_DISPLAY_SH1107:
            init = sh1107_init;
            len = sizeof(sh1107_init);
            break;
        default:
            break;
        }
        uint8_t commands[40];
        memcpy(commands, init, len);
        // Rotating is done when drawing, so the segment remap and COM scan direction are just what puts
        // the top left of the frame at the top left of the panel
        if (m_controller == MONO_DISPLAY_SH1107)
        {
            // Its COM lines run across the panel. The 64 wide panels use the middle of its 128.
            commands[len++] = 0xA8;
            commands[len++] = m_frame.panel_width() - 1;
            commands[len++] = 0xD3;
            commands[len++] = m_frame.panel_width() == 64 ? 0x60 : 0x00;
            commands[len++] = 0xA0;
            commands[len++] = 0xC0;
        }
        else
        {
            commands[len++] = 0xA8;
            commands[len++] = m_frame.panel_height() - 1;
            commands[len++] = 0xA1;
            commands[len++] = 0xC8;
            // COM pin layout, sequential on the 32 tall SSD1306 / SSD1309 panels
            commands[len++] = 0xDA;
            commands[len++] = m_frame.panel_height() == 32 && m_controller != MONO_DISPLAY_SH1106 ? 0x02 : 0x12;
        }
        commands[len++] = 0x81;
        commands[len++] = m_contrast;
        if (!send(MONO_DISPLAY_INIT, commands, len, false))
        {
            m_status = MONO_DISPLAY_IDLE;
            m_retry_at = now + MONO_DISPLAY_RETRY_MS;
        }
        return;
    }
    uint8_t page, chunk;
    if (m_check && next_chunk(page, chunk))
    {
        uint8_t col = chunk * MONO_DISPLAY_CHUNK + (m_controller == MONO_DISPLAY_SH1106 ? SH1106_COLUMN_OFFSET : 0);
        const uint8_t commands[] = {(uint8_t)(0xB0 | page), (uint8_t)(0x00 | (col & 0x0F)), (uint8_t)(0x10 | (col >> 4))};
        memcpy(m_chunk, &m_frame.pages[page][chunk * MONO_DISPLAY_CHUNK], MONO_DISPLAY_CHUNK);
        m_sending_page = page;
        m_sending_chunk = chunk;
        send(MONO_DISPLAY_CHUNK_WRITE, commands, sizeof(commands), true);
        return;
    }
    m_check = false;
    if (m_panel_on != m_on)
    {
        const uint8_t command = m_on ? 0xAF : 0xAE;
        m_sending_on = m_on;
        send(MONO_DISPLAY_POWER, &command, 1, false);
    }
}
