#include "mono_framebuffer.hpp"
#include "font5x7.h"
#include <algorithm>

static uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    return (b & 0xAA) >> 1 | (b & 0x55) << 1;
}

void MonoFramebuffer::configure(int panel_width, int panel_height, uint8_t rotation)
{
    m_panel_width = panel_width > MAX_WIDTH ? MAX_WIDTH : panel_width;
    m_panel_height = panel_height > MAX_HEIGHT ? MAX_HEIGHT : panel_height;
    m_rotation = rotation & 3;
    m_width = m_rotation & 1 ? m_panel_height : m_panel_width;
    m_height = m_rotation & 1 ? m_panel_width : m_panel_height;
    clear();
}

bool MonoFramebuffer::to_panel(int &x, int &y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
    {
        return false;
    }
    int px = x, py = y;
    switch (m_rotation)
    {
    case 1:
        px = m_panel_width - 1 - y;
        py = x;
        break;
    case 2:
        px = m_panel_width - 1 - x;
        py = m_panel_height - 1 - y;
        break;
    case 3:
        px = y;
        py = m_panel_height - 1 - x;
        break;
    }
    x = px;
    y = py;
    return true;
}

bool MonoFramebuffer::pixel(int x, int y) const
{
    if (!to_panel(x, y))
    {
        return false;
    }
    return pages[y / 8][x] & (1 << (y % 8));
}

void MonoFramebuffer::set_pixel(int x, int y, bool on)
{
    if (!to_panel(x, y))
    {
        return;
    }
    if (on)
    {
        pages[y / 8][x] |= 1 << (y % 8);
    }
    else
    {
        pages[y / 8][x] &= ~(1 << (y % 8));
    }
}

void MonoFramebuffer::fill_rect(int x, int y, int w, int h, bool on)
{
    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    if (w > m_width - x)
    {
        w = m_width - x;
    }
    if (h > m_height - y)
    {
        h = m_height - y;
    }
    if (w <= 0 || h <= 0)
    {
        return;
    }
    // A rectangle is still a rectangle on the panel, just turned
    switch (m_rotation)
    {
    case 1:
        fill_panel(m_panel_width - y - h, x, h, w, on);
        break;
    case 2:
        fill_panel(m_panel_width - x - w, m_panel_height - y - h, w, h, on);
        break;
    case 3:
        fill_panel(y, m_panel_height - x - w, h, w, on);
        break;
    default:
        fill_panel(x, y, w, h, on);
        break;
    }
}

void MonoFramebuffer::fill_panel(int x, int y, int w, int h, bool on)
{
    // A whole column of each page at a time, masked to the rows inside the rectangle
    for (int page = y / 8; page <= (y + h - 1) / 8; page++)
    {
        int top = page * 8;
        uint8_t mask = 0xFF;
        if (y > top)
        {
            mask &= 0xFF << (y - top);
        }
        if (y + h < top + 8)
        {
            mask &= 0xFF >> (top + 8 - y - h);
        }
        uint8_t *col = &pages[page][x];
        uint8_t *end = col + w;
        if (on)
        {
            for (; col < end; col++)
            {
                *col |= mask;
            }
        }
        else
        {
            mask = ~mask;
            for (; col < end; col++)
            {
                *col &= mask;
            }
        }
    }
}

void MonoFramebuffer::draw_text(int x, int y, const char *text, int scale, bool invert)
{
    if (scale < 1)
    {
        scale = 1;
    }
    if (scale > MAX_SCALE)
    {
        scale = MAX_SCALE;
    }
    for (; *text && x < m_width; text++, x += CHAR_WIDTH * scale)
    {
        uint8_t c = *text;
        if (c < FONT5X7_FIRST || c > FONT5X7_LAST)
        {
            c = '?';
        }
        const uint8_t *glyph = font5x7[c - FONT5X7_FIRST];
        for (int col = 0; col < CHAR_WIDTH; col++)
        {
            uint8_t bits = col < 5 ? glyph[col] : 0;
            if (invert)
            {
                bits = ~bits;
            }
            // Upright and full size, each column of the glyph is already a column of the panel
            if (scale == 1 && !(m_rotation & 1))
            {
                int lx = x + col;
                if (lx >= 0 && lx < m_width)
                {
                    if (m_rotation == 2)
                    {
                        write_column(m_panel_width - 1 - lx, m_panel_height - y - CHAR_HEIGHT, reverse_bits(bits),
                                     CHAR_HEIGHT);
                    }
                    else
                    {
                        write_column(lx, y, bits, CHAR_HEIGHT);
                    }
                }
                continue;
            }
            // Each run of rows that are the same is one rectangle
            int start = 0;
            for (int row = 1; row <= CHAR_HEIGHT; row++)
            {
                bool on = (bits >> start) & 1;
                if (row == CHAR_HEIGHT || ((bits >> row) & 1) != on)
                {
                    fill_rect(x + col * scale, y + start * scale, scale, (row - start) * scale, on);
                    start = row;
                }
            }
        }
    }
}

void MonoFramebuffer::write_column(int px, int py, uint8_t bits, int n)
{
    if (px < 0 || px >= m_panel_width)
    {
        return;
    }
    // Lined up with a page, which is the usual case, it's just that byte
    if (n == 8 && !(py & 7) && py >= 0 && py < m_panel_height)
    {
        pages[py / 8][px] = bits;
        return;
    }
    uint32_t mask = (1u << n) - 1;
    uint32_t value = bits & mask;
    if (py < 0)
    {
        if (py <= -8)
        {
            return;
        }
        mask >>= -py;
        value >>= -py;
        py = 0;
    }
    int page = py / 8;
    int shift = py % 8;
    mask <<= shift;
    value <<= shift;
    // The rows can straddle two pages
    for (int i = 0; i < 2 && page + i < m_panel_height / 8; i++, mask >>= 8, value >>= 8)
    {
        uint8_t *col = &pages[page + i][px];
        *col = (*col & ~mask) | (value & mask);
    }
}

void MonoFramebuffer::draw_bitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint32_t len, bool invert)
{
    if (w <= 0 || h <= 0 || w > m_width || h > m_height)
    {
        return;
    }
    uint32_t stride = (w + 7) / 8;
    uint8_t flip = invert ? 0xFF : 0;
    auto byte_at = [&](int row, int byte)
    {
        uint32_t idx = row * stride + byte;
        return idx < len ? bitmap[idx] : 0;
    };
    if (m_rotation & 1)
    {
        // Each row of the image is a column of the panel, so each byte of it is 8 pixels down that column,
        // with the first pixel at the top for a quarter turn clockwise and at the bottom for anticlockwise
        for (int row = 0; row < h; row++)
        {
            int ly = y + row;
            if (ly < 0 || ly >= m_height)
            {
                continue;
            }
            int px = m_rotation == 1 ? m_panel_width - 1 - ly : ly;
            for (uint32_t byte = 0; byte < stride; byte++)
            {
                int n = w - byte * 8 < 8 ? w - byte * 8 : 8;
                uint8_t bits = byte_at(row, byte) ^ flip;
                int lx = x + byte * 8;
                if (m_rotation == 1)
                {
                    write_column(px, lx, reverse_bits(bits), n);
                }
                else
                {
                    // The last n pixels of the byte, bottom pixel first
                    write_column(px, m_panel_height - lx - n, bits >> (8 - n), n);
                }
            }
        }
        return;
    }
    // Upright or upside down, each 8 x 8 block of the image is transposed into 8 columns of the panel
    for (int row = 0; row < h; row += 8)
    {
        int n = h - row < 8 ? h - row : 8;
        for (uint32_t byte = 0; byte < stride; byte++)
        {
            // The rows bottom first, so the transposed columns have the top row in bit 0
            uint32_t a = 0, b = 0;
            for (int i = 0; i < n; i++)
            {
                uint32_t bits = byte_at(row + i, byte);
                if (i < 4)
                {
                    b |= bits << (8 * i);
                }
                else
                {
                    a |= bits << (8 * (i - 4));
                }
            }
            // Hacker's Delight transpose8
            uint32_t t = (a ^ (a >> 7)) & 0x00AA00AA;
            a = a ^ t ^ (t << 7);
            t = (b ^ (b >> 7)) & 0x00AA00AA;
            b = b ^ t ^ (t << 7);
            t = (a ^ (a >> 14)) & 0x0000CCCC;
            a = a ^ t ^ (t << 14);
            t = (b ^ (b >> 14)) & 0x0000CCCC;
            b = b ^ t ^ (t << 14);
            t = (a & 0xF0F0F0F0) | ((b >> 4) & 0x0F0F0F0F);
            b = ((a << 4) & 0xF0F0F0F0) | (b & 0x0F0F0F0F);
            a = t;
            int cols = w - byte * 8 < 8 ? w - byte * 8 : 8;
            for (int j = 0; j < cols; j++)
            {
                uint8_t column = (j < 4 ? a >> (24 - 8 * j) : b >> (56 - 8 * j)) ^ flip;
                int lx = x + byte * 8 + j;
                if (lx < 0 || lx >= m_width)
                {
                    continue;
                }
                if (m_rotation == 2)
                {
                    // The bottom row of the image first
                    write_column(m_panel_width - 1 - lx, m_panel_height - (y + row) - n,
                                 reverse_bits(column) >> (8 - n), n);
                }
                else
                {
                    write_column(lx, y + row, column, n);
                }
            }
        }
    }
}

void MonoFramebuffer::draw_bar(int x, int y, int w, int h, uint16_t value, bool vertical, bool invert)
{
    if (w <= 0 || h <= 0 || w > m_width || h > m_height)
    {
        return;
    }
    fill_rect(x, y, w, h, invert);
    // Too small for an outline, so the whole bar fills
    int border = w >= 3 && h >= 3 ? 1 : 0;
    if (border)
    {
        draw_outline(x, y, w, h, !invert);
    }
    int inner_w = w - border * 2;
    int inner_h = h - border * 2;
    if (vertical)
    {
        int filled = ((uint32_t)inner_h * value + UINT16_MAX / 2) / UINT16_MAX;
        fill_rect(x + border, y + border + inner_h - filled, inner_w, filled, !invert);
    }
    else
    {
        int filled = ((uint32_t)inner_w * value + UINT16_MAX / 2) / UINT16_MAX;
        fill_rect(x + border, y + border, filled, inner_h, !invert);
    }
}

void MonoFramebuffer::stick_dot(int w, int h, uint16_t stick_x, uint16_t stick_y, int &dot_x, int &dot_y, int &size)
{
    // Inside the outline, a 3 pixel dot once there's room to see it move
    int inner_w = w - 2;
    int inner_h = h - 2;
    size = inner_w >= 9 && inner_h >= 9 ? 3 : 1;
    dot_x = 1 + ((uint32_t)(inner_w - size) * stick_x + UINT16_MAX / 2) / UINT16_MAX;
    dot_y = 1 + ((uint32_t)(inner_h - size) * (UINT16_MAX - stick_y) + UINT16_MAX / 2) / UINT16_MAX;
}

void MonoFramebuffer::draw_stick(int x, int y, int w, int h, uint16_t stick_x, uint16_t stick_y, bool invert)
{
    if (w < 3 || h < 3 || w > m_width || h > m_height)
    {
        return;
    }
    fill_rect(x, y, w, h, invert);
    draw_outline(x, y, w, h, !invert);
    int dot_x, dot_y, size;
    stick_dot(w, h, stick_x, stick_y, dot_x, dot_y, size);
    fill_rect(x + dot_x, y + dot_y, size, size, !invert);
}

void MonoFramebuffer::draw_outline(int x, int y, int w, int h, bool on)
{
    fill_rect(x, y, w, 1, on);
    fill_rect(x, y + h - 1, w, 1, on);
    fill_rect(x, y, 1, h, on);
    fill_rect(x + w - 1, y, 1, h, on);
}

static uint32_t isqrt(uint32_t n)
{
    uint32_t root = 0;
    for (uint32_t bit = 1u << 30; bit; bit >>= 2)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
    }
    return root;
}

void MonoFramebuffer::ellipse_span(int w, int h, int row, int &left, int &right)
{
    // A pixel is inside when its centre is, measured in half pixels from the middle of the box so it's
    // all whole numbers: (X / w)^2 + (Y / h)^2 <= 1
    int dy = 2 * row + 1 - h;
    uint32_t reach = isqrt((uint32_t)(w * w) * (uint32_t)(h * h - dy * dy)) / h;
    // The columns whose X = 2 * column + 1 - w is within reach either side
    left = (w - 1 - (int)reach + 1) / 2;
    right = (w - 1 + (int)reach) / 2;
}

void MonoFramebuffer::draw_shape(int x, int y, int w, int h, bool ellipse, bool filled, bool invert)
{
    if (w <= 0 || h <= 0 || w > m_width || h > m_height)
    {
        return;
    }
    if (invert)
    {
        fill_rect(x, y, w, h, true);
    }
    if (!ellipse)
    {
        if (filled || w < 3 || h < 3)
        {
            fill_rect(x, y, w, h, !invert);
        }
        else
        {
            draw_outline(x, y, w, h, !invert);
        }
        return;
    }
    for (int row = 0; row < h; row++)
    {
        int left, right;
        ellipse_span(w, h, row, left, right);
        if (left > right)
        {
            continue;
        }
        if (filled)
        {
            fill_rect(x + left, y + row, right - left + 1, 1, !invert);
            continue;
        }
        // The edge is what's inside this row but not the rows either side. Past the top and bottom
        // nothing is inside.
        int inner_left = w, inner_right = -1;
        if (row > 0 && row < h - 1)
        {
            int above_left, above_right, below_left, below_right;
            ellipse_span(w, h, row - 1, above_left, above_right);
            ellipse_span(w, h, row + 1, below_left, below_right);
            inner_left = std::max(above_left, below_left);
            inner_right = std::min(above_right, below_right);
        }
        if (inner_left > inner_right)
        {
            fill_rect(x + left, y + row, right - left + 1, 1, !invert);
            continue;
        }
        // The ends of the row are always edge, and so is anything past the rows either side
        int edge_left = std::max(inner_left, left + 1);
        int edge_right = std::min(inner_right, right - 1);
        fill_rect(x + left, y + row, edge_left - left, 1, !invert);
        fill_rect(x + edge_right + 1, y + row, right - edge_right, 1, !invert);
    }
}
