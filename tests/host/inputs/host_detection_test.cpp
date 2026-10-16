#include <gtest/gtest.h>
#include "managers/config_manager.hpp"

class HostDetectionTest : public ::testing::Test
{
protected:
    void SetUp() override { ConfigManager::instance().clear_all(); }
    void TearDown() override { ConfigManager::instance().clear_all(); }
};

TEST_F(HostDetectionTest, NothingIsDetectedAtStartup)
{
    EXPECT_EQ(ConfigManager::instance().get_detected_mode(), ModeHid);
}

TEST_F(HostDetectionTest, DetectingAHostRequestsItsMode)
{
    auto &config_mgr = ConfigManager::instance();
    config_mgr.host_detected(ModePs3);
    EXPECT_EQ(config_mgr.get_detected_mode(), ModePs3);
    EXPECT_EQ(config_mgr.get_requested_mode(), ModePs3);
}

TEST_F(HostDetectionTest, TheSameHostIsNotRequestedAgainOnceAProfileChoseAnotherMode)
{
    // eg a keyboard profile on a PS3 has to stay in ModeHid, which the PS3 then detects again
    auto &config_mgr = ConfigManager::instance();
    config_mgr.host_detected(ModePs3);
    config_mgr.sync_requested_mode_to_current();
    config_mgr.request_mode(ModeHid);
    config_mgr.sync_requested_mode_to_current();

    config_mgr.host_detected(ModePs3);
    EXPECT_EQ(config_mgr.get_requested_mode(), ModeHid);
    EXPECT_FALSE(config_mgr.should_reinit(0));
}

TEST_F(HostDetectionTest, ADifferentHostIsStillRequested)
{
    auto &config_mgr = ConfigManager::instance();
    config_mgr.host_detected(ModePs3);
    config_mgr.sync_requested_mode_to_current();
    config_mgr.request_mode(ModeHid);
    config_mgr.sync_requested_mode_to_current();

    config_mgr.host_detected(ModeSwitch);
    EXPECT_EQ(config_mgr.get_detected_mode(), ModeSwitch);
    EXPECT_EQ(config_mgr.get_requested_mode(), ModeSwitch);
}
