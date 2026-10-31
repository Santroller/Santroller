#pragma once
#include <stdint.h>
#include <string.h>

// A monochrome frame, laid out like the SSD1306 / SH1106 / SH1107 RAM: pages of 8 rows, each byte a column
// of 8 pixels with the top one in bit 0. Drawing is in the frame's own coordinates, which are rotated onto
// the panel, so a portrait panel can be used sideways.
class MonoFramebuffer
{
public:
    static constexpr int MAX_WIDTH = 128;
    static constexpr int MAX_HEIGHT = 128;
    static constexpr int MAX_PAGES = MAX_HEIGHT / 8;
    // Each character takes up this much room, the glyphs being 5 x 7 with a gap right and below
    static constexpr int GLYPH_WIDTH = 6;
    static constexpr int GLYPH_HEIGHT = 8;
    static constexpr int MAX_SCALE = 8;

    MonoFramebuffer() { configure(128, 64, 0); }
    // The panel's size, and how many quarter turns clockwise what is drawn is rotated by on it
    void configure(int panel_width, int panel_height, uint8_t rotation);
    void clear() { memset(pages, 0, sizeof(pages)); }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int panel_width() const { return m_panel_width; }
    int panel_height() const { return m_panel_height; }
    bool pixel(int x, int y) const;
    void set_pixel(int x, int y, bool on);
    void fill_rect(int x, int y, int w, int h, bool on);
    // Every pixel of the box is drawn, so it covers whatever was there
    void draw_text(int x, int y, const char *text, int scale, bool invert);
    // bitmap is row by row, each row starting on a new byte, most significant bit first
    void draw_bitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint32_t len, bool invert);
    // An outline, filled from the left (or the bottom) in proportion to value out of UINT16_MAX
    void draw_bar(int x, int y, int w, int h, uint16_t value, bool vertical, bool invert);
    // An outline with a dot where a joystick is, larger values right and up
    void draw_stick(int x, int y, int w, int h, uint16_t stick_x, uint16_t stick_y, bool invert);
    // Where draw_stick puts the dot in its w x h box, and how big it is
    static void stick_dot(int w, int h, uint16_t stick_x, uint16_t stick_y, int &dot_x, int &dot_y, int &size);
    // A w x h rectangle, or the ellipse that fits in it, filled or as a 1 pixel outline
    void draw_shape(int x, int y, int w, int h, bool ellipse, bool filled, bool invert);
    // The columns of row (0 to h - 1) inside the ellipse that fits a w x h box, none when left > right
    static void ellipse_span(int w, int h, int row, int &left, int &right);

    // Indexed by the panel's page and column
    uint8_t pages[MAX_PAGES][MAX_WIDTH];

private:
    // Where (x, y) is on the panel, false when it's off the edge
    bool to_panel(int &x, int &y) const;
    void draw_outline(int x, int y, int w, int h, bool on);
    // Already on the panel and clipped to it
    void fill_panel(int x, int y, int w, int h, bool on);
    // Sets n (up to 8) rows of a panel column from py down to the low n bits of bits, bit 0 at the top
    void write_column(int px, int py, uint8_t bits, int n);
    int m_width;
    int m_height;
    int m_panel_width;
    int m_panel_height;
    uint8_t m_rotation;
};
