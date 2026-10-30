#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "mono_framebuffer.hpp"
#include "font5x7.h"
#include <string.h>

// The config tool's preview (src/components/Inputs/displayRender.ts) expects these same frames
namespace
{
std::vector<std::string> rows(const MonoFramebuffer &frame, int width, int height)
{
    std::vector<std::string> out;
    for (int y = 0; y < height; y++)
    {
        std::string row;
        for (int x = 0; x < width; x++)
        {
            row += frame.pixel(x, y) ? '#' : '.';
        }
        out.push_back(row);
    }
    return out;
}

MonoFramebuffer blank()
{
    MonoFramebuffer frame;
    frame.clear();
    return frame;
}
} // namespace

TEST(MonoFramebuffer, DrawsText)
{
    auto frame = blank();
    frame.draw_text(0, 0, "A1", 1, false);
    EXPECT_EQ(rows(frame, 12, 8), (std::vector<std::string>{
                                      ".###....#...",
                                      "#...#..##...",
                                      "#...#...#...",
                                      "#...#...#...",
                                      "#####...#...",
                                      "#...#...#...",
                                      "#...#..###..",
                                      "............",
                                  }));
}

TEST(MonoFramebuffer, InvertsTheWholeCharacterCell)
{
    auto frame = blank();
    frame.draw_text(0, 0, " ", 2, true);
    std::vector<std::string> expected(16, "############.");
    expected.push_back(".............");
    EXPECT_EQ(rows(frame, 13, 17), expected);
}

TEST(MonoFramebuffer, FillsBarsInProportion)
{
    auto frame = blank();
    frame.draw_bar(0, 0, 10, 4, 32768, false, false);
    EXPECT_EQ(rows(frame, 10, 4), (std::vector<std::string>{"##########", "#####....#", "#####....#", "##########"}));
    auto vertical = blank();
    vertical.draw_bar(0, 0, 3, 6, UINT16_MAX / 2, true, false);
    EXPECT_EQ(rows(vertical, 3, 6), (std::vector<std::string>{"###", "#.#", "#.#", "###", "###", "###"}));
}

TEST(MonoFramebuffer, DrawsBitmapsRowByRow)
{
    auto frame = blank();
    const uint8_t image[] = {0b10100000, 0b01000000};
    frame.draw_bitmap(1, 1, 3, 2, image, sizeof(image), false);
    EXPECT_EQ(rows(frame, 5, 4), (std::vector<std::string>{".....", ".#.#.", "..#..", "....."}));
}

TEST(MonoFramebuffer, ShortImageDataDrawsDark)
{
    auto frame = blank();
    frame.fill_rect(0, 0, 8, 2, true);
    const uint8_t image[] = {0xFF};
    frame.draw_bitmap(0, 0, 8, 2, image, sizeof(image), false);
    EXPECT_EQ(rows(frame, 8, 2), (std::vector<std::string>{"########", "........"}));
}

TEST(MonoFramebuffer, ClipsAtTheEdges)
{
    auto frame = blank();
    frame.draw_text(120, 60, "Hello", 8, false);
    frame.fill_rect(-1000, -1000, 100000, 100000, true);
    for (int page = 0; page < MonoFramebuffer::MAX_PAGES; page++)
    {
        for (int col = 0; col < MonoFramebuffer::MAX_WIDTH; col++)
        {
            // Only the panel's own pages are drawn on
            ASSERT_EQ(frame.pages[page][col], page < 8 ? 0xFF : 0x00);
        }
    }
}

TEST(MonoFramebuffer, PacksPagesLikeTheController)
{
    auto frame = blank();
    frame.set_pixel(5, 0, true);
    frame.set_pixel(5, 9, true);
    EXPECT_EQ(frame.pages[0][5], 0x01);
    EXPECT_EQ(frame.pages[1][5], 0x02);
}

TEST(MonoFramebuffer, RotatesOntoThePanel)
{
    // A 64 x 128 SH1107 panel used sideways
    MonoFramebuffer frame;
    frame.configure(64, 128, 1);
    EXPECT_EQ(frame.width(), 128);
    EXPECT_EQ(frame.height(), 64);
    // A quarter turn clockwise: the top left corner is at the panel's top right
    frame.set_pixel(0, 0, true);
    EXPECT_EQ(frame.pages[0][63], 0x01);
    // and the bottom right at its bottom left
    frame.set_pixel(127, 63, true);
    EXPECT_EQ(frame.pages[15][0], 0x80);
    EXPECT_TRUE(frame.pixel(127, 63));
    EXPECT_FALSE(frame.pixel(128, 0));

    MonoFramebuffer upside_down;
    upside_down.configure(128, 64, 2);
    upside_down.set_pixel(0, 0, true);
    EXPECT_EQ(upside_down.pages[7][127], 0x80);

    MonoFramebuffer anticlockwise;
    anticlockwise.configure(128, 128, 3);
    anticlockwise.set_pixel(0, 0, true);
    EXPECT_EQ(anticlockwise.pages[15][0], 0x80);
}

TEST(MonoFramebuffer, RotatedTextReadsTheSameWay)
{
    MonoFramebuffer straight;
    straight.draw_text(3, 2, "Hi", 2, false);
    MonoFramebuffer rotated;
    rotated.configure(64, 128, 1);
    rotated.draw_text(3, 2, "Hi", 2, false);
    for (int y = 0; y < 64; y++)
    {
        for (int x = 0; x < 128; x++)
        {
            ASSERT_EQ(straight.pixel(x, y), rotated.pixel(x, y)) << x << "," << y;
        }
    }
}

// The fast paths (whole bytes at a time, runs of pixels) against drawing pixel by pixel, for every panel
// and rotation, with shapes hanging off every edge
namespace
{
void reference_fill(MonoFramebuffer &frame, int x, int y, int w, int h, bool on)
{
    for (int j = y; j < y + h; j++)
    {
        for (int i = x; i < x + w; i++)
        {
            frame.set_pixel(i, j, on);
        }
    }
}

void reference_bitmap(MonoFramebuffer &frame, int x, int y, int w, int h, const uint8_t *bitmap, uint32_t len, bool invert)
{
    uint32_t stride = (w + 7) / 8;
    for (int row = 0; row < h; row++)
    {
        for (int col = 0; col < w; col++)
        {
            uint32_t idx = row * stride + col / 8;
            bool on = idx < len && (bitmap[idx] & (0x80 >> (col % 8)));
            frame.set_pixel(x + col, y + row, on != invert);
        }
    }
}

void expect_same(const MonoFramebuffer &a, const MonoFramebuffer &b, int trial)
{
    ASSERT_EQ(memcmp(a.pages, b.pages, sizeof(a.pages)), 0) << "trial " << trial;
}
} // namespace

TEST(MonoFramebuffer, FastPathsMatchDrawingPixelByPixel)
{
    const int panels[][2] = {{128, 64}, {128, 32}, {64, 128}, {128, 128}};
    uint32_t seed = 12345;
    auto rand = [&seed](int max)
    {
        seed = seed * 1103515245 + 12345;
        return (int)((seed >> 8) % max);
    };
    int trial = 0;
    for (auto &panel : panels)
    {
        for (uint8_t rotation = 0; rotation < 4; rotation++)
        {
            for (int i = 0; i < 300; i++, trial++)
            {
                MonoFramebuffer fast, slow;
                fast.configure(panel[0], panel[1], rotation);
                slow.configure(panel[0], panel[1], rotation);
                // Some noise underneath, so clearing pixels is checked too
                for (int n = 0; n < 200; n++)
                {
                    int px = rand(fast.width()), py = rand(fast.height());
                    fast.set_pixel(px, py, true);
                    slow.set_pixel(px, py, true);
                }
                int x = rand(fast.width() + 40) - 20, y = rand(fast.height() + 40) - 20;
                int w = rand(fast.width()) + 1, h = rand(fast.height()) + 1;
                bool on = rand(2);
                fast.fill_rect(x, y, w, h, on);
                reference_fill(slow, x, y, w, h, on);
                expect_same(fast, slow, trial);

                uint8_t bitmap[2048];
                for (auto &b : bitmap)
                {
                    b = rand(4) ? 0x00 : rand(256);
                }
                uint32_t len = rand(sizeof(bitmap));
                bool invert = rand(2);
                fast.draw_bitmap(x, y, w, h, bitmap, len, invert);
                reference_bitmap(slow, x, y, w, h, bitmap, len, invert);
                expect_same(fast, slow, trial);
            }
        }
    }
}

TEST(MonoFramebuffer, TextMatchesItsGlyphsInEveryRotation)
{
    for (uint8_t rotation = 0; rotation < 4; rotation++)
    {
        for (int scale = 1; scale <= 3; scale++)
        {
            for (bool invert : {false, true})
            {
                MonoFramebuffer frame;
                frame.configure(128, 128, rotation);
                frame.draw_text(-3, 5, "Ag!~", scale, invert);
                for (int i = 0; i < 4; i++)
                {
                    const char c = "Ag!~"[i];
                    for (int col = 0; col < MonoFramebuffer::CHAR_WIDTH * scale; col++)
                    {
                        uint8_t bits = col / scale < 5 ? font5x7[c - FONT5X7_FIRST][col / scale] : 0;
                        for (int row = 0; row < MonoFramebuffer::CHAR_HEIGHT * scale; row++)
                        {
                            int x = -3 + i * MonoFramebuffer::CHAR_WIDTH * scale + col;
                            if (x < 0)
                            {
                                continue;
                            }
                            bool expected = ((bits >> (row / scale)) & 1) != invert;
                            ASSERT_EQ(frame.pixel(x, 5 + row), expected)
                                << "rotation " << (int)rotation << " scale " << scale << " " << c;
                        }
                    }
                }
            }
        }
    }
}

TEST(MonoFramebuffer, TextMatchesDrawingPixelByPixelAnywhere)
{
    const int panels[][2] = {{128, 64}, {128, 32}, {64, 128}, {128, 128}};
    uint32_t seed = 99;
    auto rand = [&seed](int max)
    {
        seed = seed * 1103515245 + 12345;
        return (int)((seed >> 8) % max);
    };
    for (auto &panel : panels)
    {
        for (uint8_t rotation = 0; rotation < 4; rotation++)
        {
            for (int trial = 0; trial < 200; trial++)
            {
                MonoFramebuffer fast, slow;
                fast.configure(panel[0], panel[1], rotation);
                slow.configure(panel[0], panel[1], rotation);
                for (int n = 0; n < 200; n++)
                {
                    int px = rand(fast.width()), py = rand(fast.height());
                    fast.set_pixel(px, py, true);
                    slow.set_pixel(px, py, true);
                }
                char text[6];
                for (int i = 0; i < 5; i++)
                {
                    text[i] = ' ' + rand(95);
                }
                text[5] = 0;
                int x = rand(fast.width() + 40) - 30, y = rand(fast.height() + 30) - 20;
                int scale = rand(4) ? 1 : 2;
                bool invert = rand(2);
                fast.draw_text(x, y, text, scale, invert);
                for (int i = 0; i < 5; i++)
                {
                    for (int col = 0; col < MonoFramebuffer::CHAR_WIDTH * scale; col++)
                    {
                        uint8_t bits = col / scale < 5 ? font5x7[text[i] - FONT5X7_FIRST][col / scale] : 0;
                        for (int row = 0; row < MonoFramebuffer::CHAR_HEIGHT * scale; row++)
                        {
                            slow.set_pixel(x + i * MonoFramebuffer::CHAR_WIDTH * scale + col, y + row,
                                           ((bits >> (row / scale)) & 1) != invert);
                        }
                    }
                }
                ASSERT_EQ(memcmp(fast.pages, slow.pages, sizeof(fast.pages)), 0)
                    << "rotation " << (int)rotation << " x " << x << " y " << y << " scale " << scale;
            }
        }
    }
}
