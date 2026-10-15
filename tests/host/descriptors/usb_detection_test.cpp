#include <gtest/gtest.h>
#include "emulation/usb/usb_descriptors.h"

// Comfortably past the inference delay after a device descriptor read at 1000
static constexpr uint32_t LATER = 1000 + 5001;

TEST(UsbDetection, ConfigChangesDisableInferenceUntilRebootButOrdinaryReloadsDoNot)
{
    auto &detection = UsbDetectionState::instance();
    detection.reset();
    detection.mark_device_descriptor_read(1000);
    EXPECT_TRUE(detection.should_infer_console(LATER));

    detection.mark_device_string_read();
    EXPECT_FALSE(detection.should_infer_console(LATER));
    detection.reset();
    detection.mark_device_descriptor_read(1000);
    EXPECT_TRUE(detection.should_infer_console(LATER));

    detection.mark_hid_descriptor_read();
    EXPECT_TRUE(detection.should_infer_console(LATER));
    detection.disable_console_inference();
    EXPECT_FALSE(detection.should_infer_console(LATER));

    // A reload may restart the stack and clear all enumeration observations.
    detection.reset();
    detection.mark_device_descriptor_read(1000);
    EXPECT_FALSE(detection.seen_hid_descriptor_read());
    EXPECT_FALSE(detection.should_infer_console(LATER));

    detection.mark_hid_descriptor_read();
    EXPECT_FALSE(detection.should_infer_console(LATER));
    detection.reset();
    EXPECT_FALSE(detection.should_infer_console(LATER));
}

TEST(UsbDetection, InferenceWaitsForDelayAfterFirstDeviceDescriptorRead)
{
    // Inference may already be disabled by the test above (it is a singleton), so this only checks the negatives
    auto &detection = UsbDetectionState::instance();
    detection.reset();

    // Nothing has enumerated us yet, however long we have been up
    EXPECT_FALSE(detection.should_infer_console(100000));

    // A host that starts enumerating late still gets the full delay to read strings
    detection.mark_device_descriptor_read(100000);
    detection.mark_device_descriptor_read(102000);
    EXPECT_FALSE(detection.should_infer_console(100001));
    EXPECT_FALSE(detection.should_infer_console(105000));

    detection.mark_device_string_read();
    EXPECT_FALSE(detection.should_infer_console(200000));

    // The first read time is cleared along with everything else
    detection.reset();
    EXPECT_FALSE(detection.should_infer_console(200000));
}
