#include <gtest/gtest.h>
#include "managers/config_manager.hpp"

class FirmwareUpdateTest : public ::testing::Test
{
protected:
    void SetUp() override { ConfigManager::instance().clear_all(); }
    void TearDown() override { ConfigManager::instance().clear_all(); }
};

TEST_F(FirmwareUpdateTest, ScheduledReloadWaitsUntilUploadEnds)
{
    auto &manager = ConfigManager::instance();
    manager.schedule_reinit(100);
    manager.touch_firmware_update(50);

    EXPECT_FALSE(manager.should_reinit(200));
    EXPECT_EQ(manager.get_reinit_time(), 100u);

    manager.finish_firmware_update();
    EXPECT_TRUE(manager.should_reinit(200));
}

TEST_F(FirmwareUpdateTest, ModeChangesDuringUploadAreDeferredNotLost)
{
    auto &manager = ConfigManager::instance();
    manager.touch_firmware_update(50);
    manager.host_detected(ModePs3);

    EXPECT_FALSE(manager.should_reinit(200));
    EXPECT_EQ(manager.get_current_mode(), ModeHid);
    EXPECT_EQ(manager.get_requested_mode(), ModePs3);

    manager.finish_firmware_update();
    EXPECT_TRUE(manager.should_reinit(200));
}

TEST_F(FirmwareUpdateTest, DeviceStackRestartRemainsQueuedDuringUpload)
{
    auto &manager = ConfigManager::instance();
    manager.request_device_stack_restart();
    manager.touch_firmware_update(50);

    EXPECT_FALSE(manager.take_device_stack_restart());
    EXPECT_FALSE(manager.take_device_stack_restart());

    manager.finish_firmware_update();
    EXPECT_TRUE(manager.take_device_stack_restart());
    EXPECT_FALSE(manager.take_device_stack_restart());
}

TEST_F(FirmwareUpdateTest, OnlyFirmwarePacketsExtendTheUploadTimeout)
{
    auto &manager = ConfigManager::instance();
    manager.touch_firmware_update(100);
    EXPECT_FALSE(manager.firmware_update_stalled(10100, 10000));
    EXPECT_TRUE(manager.firmware_update_stalled(10101, 10000));

    manager.touch_firmware_update(10000);
    EXPECT_FALSE(manager.firmware_update_stalled(10101, 10000));
    EXPECT_TRUE(manager.firmware_update_stalled(20001, 10000));

    manager.finish_firmware_update();
    EXPECT_FALSE(manager.is_updating_firmware());
    EXPECT_FALSE(manager.firmware_update_stalled(30000, 10000));
}

TEST_F(FirmwareUpdateTest, UploadTimeoutWorksAcrossClockWrap)
{
    auto &manager = ConfigManager::instance();
    manager.touch_firmware_update(UINT32_MAX - 100);
    EXPECT_FALSE(manager.firmware_update_stalled(9899, 10000));
    EXPECT_TRUE(manager.firmware_update_stalled(9900, 10000));
}

TEST_F(FirmwareUpdateTest, ConfigWriteAndFirmwareUploadHaveSeparateLifetimes)
{
    auto &manager = ConfigManager::instance();
    manager.touch_firmware_update(100);
    EXPECT_FALSE(manager.is_working());

    manager.set_working(false);
    EXPECT_TRUE(manager.is_updating_firmware());
    manager.finish_firmware_update();
    EXPECT_FALSE(manager.should_reinit(200));

    manager.touch_firmware_update(300);
    manager.clear_all();
    EXPECT_FALSE(manager.is_updating_firmware());
}
