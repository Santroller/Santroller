#include <gtest/gtest.h>
#include "libmatrix.hpp"
#include <array>
#include <vector>
#include <utility>

namespace {
struct Pin {
    bool output = false;
    bool level = false;
    bool up = false;
    bool down = false;
};
std::array<Pin, NUM_BANK0_GPIOS> pins;
std::vector<std::pair<unsigned int, unsigned int>> pressed;
bool active_low;
uint64_t delay_us;

template <typename F>
void each_pin(uint64_t mask, F fn) {
    for (unsigned int pin = 0; pin < pins.size(); pin++) {
        if (mask & (1ull << pin)) {
            fn(pins[pin]);
        }
    }
}

class MatrixScan : public testing::Test {
protected:
    void SetUp() override {
        pins = {};
        pressed.clear();
        active_low = true;
        delay_us = 0;
    }
};
}

void gpio_init_mask64(uint64_t mask) {
    each_pin(mask, [](Pin &pin) { pin = {}; });
}
void gpio_set_dir_in_masked64(uint64_t mask) {
    each_pin(mask, [](Pin &pin) { pin.output = false; });
}
void gpio_set_dir_out_masked64(uint64_t mask) {
    each_pin(mask, [](Pin &pin) { pin.output = true; });
}
void gpio_put_masked64(uint64_t mask, uint64_t value) {
    for (unsigned int pin = 0; pin < pins.size(); pin++) {
        if (mask & (1ull << pin)) {
            pins[pin].level = value & (1ull << pin);
        }
    }
}
void gpio_set_pulls(unsigned int pin, bool up, bool down) {
    pins[pin].up = up;
    pins[pin].down = down;
}
bool gpio_get(unsigned int pin) {
    for (const auto &[out, in] : pressed) {
        // Model diodes that only conduct when the row is driven in the wiring's polarity.
        if (in == pin && pins[out].output && pins[out].level != active_low) {
            return pins[out].level;
        }
    }
    EXPECT_NE(pins[pin].up, pins[pin].down);
    return pins[pin].up;
}
void sleep_us(uint64_t us) {
    delay_us += us;
}

TEST_F(MatrixScan, LegacyUsesPullUpsAndIdleHighOutputs) {
    Matrix matrix((1ull << 14) | (1ull << 15), (1ull << 9) | (1ull << 10), true);
    EXPECT_TRUE(pins[14].up);
    EXPECT_FALSE(pins[14].down);
    EXPECT_TRUE(pins[15].up);
    EXPECT_TRUE(pins[9].level);
    EXPECT_TRUE(pins[10].level);
    EXPECT_FALSE(matrix.read(10, 14));
    EXPECT_EQ(delay_us, 1);
    EXPECT_TRUE(pins[9].level);
    EXPECT_TRUE(pins[10].level);
    EXPECT_FALSE(pins[9].output);
    EXPECT_FALSE(pins[10].output);
}

TEST_F(MatrixScan, RedDoesNotActivateGreenOrTheOtherFretRow) {
    Matrix matrix((1ull << 14) | (1ull << 15), (1ull << 9) | (1ull << 10), true);
    pressed = {{10, 14}};
    EXPECT_FALSE(matrix.read(10, 15));
    EXPECT_TRUE(matrix.read(10, 14));
    EXPECT_FALSE(matrix.read(9, 14));
    EXPECT_FALSE(matrix.read(9, 15));
    EXPECT_TRUE(matrix.read(10, 14));
    pressed.clear();
    EXPECT_FALSE(matrix.read(10, 14));
    EXPECT_FALSE(matrix.read(10, 15));
}

TEST_F(MatrixScan, ReadsChordsAndHighGpioPins) {
    Matrix matrix((1ull << 14) | (1ull << 47), (1ull << 10) | (1ull << 40), true);
    pressed = {{10, 14}, {40, 47}};
    EXPECT_TRUE(matrix.read(10, 14));
    EXPECT_TRUE(matrix.read(40, 47));
    EXPECT_FALSE(matrix.read(10, 47));
    EXPECT_FALSE(matrix.read(40, 14));
    EXPECT_TRUE(pins[47].up);
    EXPECT_TRUE(pins[40].level);
    EXPECT_FALSE(pins[40].output);
}

TEST_F(MatrixScan, DefaultRetainsActiveHighScanningWithPullDowns) {
    active_low = false;
    Matrix matrix((1ull << 14) | (1ull << 15), (1ull << 9) | (1ull << 10));
    EXPECT_FALSE(pins[14].up);
    EXPECT_TRUE(pins[14].down);
    pressed = {{10, 14}};
    EXPECT_TRUE(matrix.read(10, 14));
    EXPECT_FALSE(matrix.read(10, 15));
    EXPECT_FALSE(matrix.read(9, 14));
    EXPECT_FALSE(pins[10].level);
    EXPECT_FALSE(pins[10].output);
}
