#include <gtest/gtest.h>
#include "emulation/usb/usb_descriptors.h"

TEST(UsbDetection, ConfigChangesDisableInferenceUntilRebootButOrdinaryReloadsDoNot)
{
    auto &detection = UsbDetectionState::instance();
    detection.reset();
    EXPECT_TRUE(detection.should_infer_console());

    detection.mark_device_string_read();
    EXPECT_FALSE(detection.should_infer_console());
    detection.reset();
    EXPECT_TRUE(detection.should_infer_console());

    detection.mark_hid_descriptor_read();
    EXPECT_TRUE(detection.should_infer_console());
    detection.disable_console_inference();
    EXPECT_FALSE(detection.should_infer_console());

    // A reload may restart the stack and clear all enumeration observations.
    detection.reset();
    EXPECT_FALSE(detection.seen_hid_descriptor_read());
    EXPECT_FALSE(detection.should_infer_console());

    detection.mark_hid_descriptor_read();
    EXPECT_FALSE(detection.should_infer_console());
    detection.reset();
    EXPECT_FALSE(detection.should_infer_console());
}
