#pragma once
#include <cstdint>

constexpr uint16_t PDLOADER_VID = 0x0e8f;
constexpr uint16_t PDLOADER_PID = 0x2213;
constexpr uint8_t PDLOADER_PACKET_SIZE = 64;
constexpr uint8_t PDLOADER_LED_REPORT_SIZE = 100;

struct PDLoaderInputReport {
    uint8_t vendor[3] = {0x42, 0x56, 0x5a};
    uint8_t buttons1 = 0;
    uint8_t buttons2 = 0;
    uint8_t buttons3_slider1 = 0;
    uint8_t slider2 = 0;
    uint8_t slider3 = 0;
    uint8_t slider4 = 0;
    uint8_t slider5 = 0;
    uint8_t unknown[14] = {};

    static constexpr uint32_t reverse_touch_bits(uint32_t touches) {
        touches = ((touches & 0xaaaaaaaa) >> 1) | ((touches & 0x55555555) << 1);
        touches = ((touches & 0xcccccccc) >> 2) | ((touches & 0x33333333) << 2);
        touches = ((touches & 0xf0f0f0f0) >> 4) | ((touches & 0x0f0f0f0f) << 4);
        touches = ((touches & 0xff00ff00) >> 8) | ((touches & 0x00ff00ff) << 8);
        return (touches >> 16) | (touches << 16);
    }

    constexpr uint32_t slider_touches() const {
        uint32_t packed = (buttons3_slider1 >> 4) |
            (uint32_t(slider2) << 4) | (uint32_t(slider3) << 12) |
            (uint32_t(slider4) << 20) | (uint32_t(slider5 & 0x0f) << 28);
        return reverse_touch_bits(packed);
    }

    constexpr void set_slider_touches(uint32_t touches) {
        uint32_t packed = reverse_touch_bits(touches);
        buttons3_slider1 = (buttons3_slider1 & 0x0f) | ((packed & 0x0f) << 4);
        slider2 = packed >> 4;
        slider3 = packed >> 12;
        slider4 = packed >> 20;
        slider5 = packed >> 28;
    }
} __attribute__((packed));
static_assert(sizeof(PDLoaderInputReport) == 24, "PD-Loader input is 24 bytes");
static_assert([] {
    PDLoaderInputReport report;
    report.buttons3_slider1 = 1;
    report.set_slider_touches(0x80008001);
    return report.vendor[0] == 0x42 && report.vendor[1] == 0x56 &&
        report.vendor[2] == 0x5a && report.buttons1 == 0 &&
        report.slider_touches() == 0x80008001 && (report.buttons3_slider1 & 1);
}(), "PD-Loader slider must round-trip without changing button or report defaults");
