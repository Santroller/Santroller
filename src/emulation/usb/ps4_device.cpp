#include "emulation/usb/ps4_device.h"
#include "managers/battery_manager.hpp"

// Battery level and cable state, as a DualShock 4 reports them
#define PS4_STATUS_BYTE 30
#include "usb/auth_broker.h"

class UsbHostInterface;
#include <memory>
#include "managers/profile_manager.hpp"
#include "protocols/ps4.hpp"
#include "enums.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "hid_reports.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb/host/hid/hid_host.h"

// 61/1024 °/s and 1/8192 g per count (PS4_IMU_GYRO_PER_DPS and PS4_IMU_ACCEL_PER_G)
static void ps4_set_motion_scale(PS4FeatureReport &report)
{
    report.timestamp_tick_us = 4000;
    report.gyro_multiplier = 61;
    report.gyro_divisor = 1024;
    report.accel_multiplier = 1;
    report.accel_divisor = 8192;
}

static PS4FeatureReport ps4_feature_report(SubType subtype)
{
    PS4FeatureReport report = {};
    report.report_id = ReportId::ReportIdPs4Feature;
    report.magic = PS4_FEATURE_MAGIC;
    report.protocol_version = 0x04;
    report.left_stick_dead_zone = 0x0d;
    report.right_stick_dead_zone = 0x0d;
    if (subtype == LiveGuitar)
    {
        // Matches a real GHL guitar, which only says that it is a guitar
        report.device_type = PS4DeviceGuitar;
        return report;
    }
    report.touch_pad_density_integer = 44;
    report.touch_pad_density_fraction = 86;
    switch (subtype)
    {
    case Gamepad:
        report.device_type = PS4DeviceStandard;
        report.motion_sensors = true;
        report.light_bar = true;
        report.vibration = true;
        report.touch_pad = true;
        ps4_set_motion_scale(report);
        break;
    case GuitarHeroGuitar:
    case RockBandGuitar:
        // Matches a real PS4 guitar, which lacks the pickup selector that RB guitars have
        report.device_type = PS4DeviceGuitar;
        report.analog_pressure = true;
        report.extended_data = true;
        report.wireless = true;
        report.guitar.selector_switch = subtype == RockBandGuitar;
        report.guitar.tilt = true;
        report.guitar.whammy_bar = true;
        break;
    case GuitarHeroDrums:
    case RockBandDrums:
        // Matches a real PS4 drum kit
        report.device_type = PS4DeviceDrums;
        report.analog_pressure = true;
        report.extended_data = true;
        report.drums.four_pads = true;
        report.drums.hi_hat = true;
        report.drums.ride = true;
        report.drums.crash = true;
        report.drums.extra_foot_pedal = true;
        break;
    default:
        report.device_type = subtype == FightStick ? PS4DeviceStick : PS4DeviceStandard;
        report.analog_pressure = true;
        report.extended_data = true;
        report.audio_jack = true;
        ps4_set_motion_scale(report);
        break;
    }
    return report;
}

AuthPageSizeReport ps4_pagesize_report = {
    type : 0xF3,
    u1 : 0x00,
    size_challenge : 0x38,
    size_response : 0x38,
    u4 : {0x00, 0x00, 0x00, 0x00}
};

uint8_t const desc_hid_report_ps4[] =
    {
        TUD_HID_REPORT_DESC_PS4_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

PS4GamepadDevice::PS4GamepadDevice()
{
}
void PS4GamepadDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);

    PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)m_initial_report;
    memset(gamepad, 0, sizeof(PS4Dpad_Data_t));
    gamepad->report_id = 1;
    gamepad->leftStickX = PS3_STICK_CENTER;
    gamepad->leftStickY = PS3_STICK_CENTER;
    gamepad->rightStickX = PS3_STICK_CENTER;
    gamepad->rightStickY = PS3_STICK_CENTER;
    gamepad->reportCounter = 1;
}
void PS4GamepadDevice::process(bool full_poll, bool send_events)
{
    if (!tud_ready())
    {
        for (const auto &profile : profiles)
        {
            for (const auto &led : profile->leds)
            {
                led->update(full_poll, send_events);
            }
        }
        return;
    }
    if (!ready())
        return;
    memcpy(epin_buf, &m_initial_report, sizeof(m_initial_report));
    MotionState motion;
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_ps4(epin_buf);
            mapping->update_motion(motion);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)epin_buf;
    if (subtype == Gamepad)
    {
        // in the scale feature report 0x03 states
        gamepad->imu = ps_imu_encode(motion, PS4_IMU_GYRO_PER_DPS, PS4_IMU_ACCEL_PER_G);
        gamepad->sensorTimestamp = (uint16_t)((uint64_t)time_us_32() * 3 / 16);
    }
    if (!m_sent_first_report)
    {
        m_sent_first_report = true;
        gamepad->guide = true;
    }
    // convert bitmask dpad to actual hid dpad
    gamepad->dpad = GamepadButtonMapping::dpad_bindings[gamepad->dpad];
    // every PS4 report puts the battery and cable state at the same byte as a DualShock 4
    epin_buf[PS4_STATUS_BYTE] = BatteryManager::instance().ps4_status();
    send_report(sizeof(PS4Dpad_Data_t), 0, epin_buf);
    PS4Dpad_Data_t *initial = (PS4Dpad_Data_t *)m_initial_report;
    initial->reportCounter++;
    if (initial->reportCounter == 0) {
        initial->reportCounter = 1;
    }
}

size_t PS4GamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t PS4GamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps4), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t PS4GamepadDevice::device_name(uint8_t idx, char *desc) 
{
    return 0;
}

void PS4GamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    switch (subtype)
    {
    case ProjectDiva:
        desc->idVendor = HORI_VID;
        desc->idProduct = 0x013c;
        break;
    case Taiko:
        desc->idVendor = HORI_VID;
        desc->idProduct = HORI_PS4_TAIKO_PID;
        break;
    case Dancepad:
        desc->idVendor = 0x1532;
        desc->idProduct = 0x0401;
        break;
    default:
        break;
    }
}
const uint8_t *PS4GamepadDevice::report_descriptor()
{
    return desc_hid_report_ps4;
}

uint16_t PS4GamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_ps4);
}

uint16_t PS4GamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    if (report_type != HID_REPORT_TYPE_FEATURE)
    {
        return 0;
    }
    bool status;
    std::shared_ptr<HidHost> host_device;
    // PS4 auth uses HID feature reports directly, not XGIP packets
    // So we still need direct access to the host device here
    auto auth_device = auth_broker.get_auth_device(ModePs4);
    if (auth_device)
    {
        host_device = std::static_pointer_cast<HidHost>(auth_device);
    }

    switch (report_id)
    {
    case ReportId::ReportIdPs4Feature:
    {
        PS4FeatureReport report = ps4_feature_report(subtype);
        memcpy(buffer, &report, sizeof(report));
        return sizeof(report);
    }
    case ReportId::ReportIdPs4GetResponse:
    case ReportId::ReportIdPs4GetAuthStatus:
        if (host_device)
        {
            return host_device->get_report(report_id, report_type, buffer, reqlen, nullptr);
        }
        return 0;
    case ReportId::ReportIdPs4GetAuthPageSize:

        if (host_device)
        {
            status = false;
            reqlen = host_device->get_report(report_id, report_type, buffer, reqlen, &status);
            if (status)
            {
                return reqlen;
            }
        }
        // try to pass through to ps4 over usb host if one exists
        // if it is a ds4 the request will fail but we need to return this one in that case
        memcpy(buffer, &ps4_pagesize_report, sizeof(ps4_pagesize_report));
        return sizeof(ps4_pagesize_report);
    }
    return 0;
}

void PS4GamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    std::shared_ptr<HidHost> host_device;
    // PS4 auth uses HID feature reports directly
    auto auth_device = auth_broker.get_auth_device(ModePs4);
    if (auth_device)
    {
        host_device = std::static_pointer_cast<HidHost>(auth_device);
    }
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
    {
        switch (report_id)
        {
        case 0:
            return;
        case ReportId::ReportIdPs4SetChallenge:
            if (host_device)
            {
                host_device->set_report(report_id, report_type, (uint8_t *)buffer, bufsize, nullptr);
            }
            return;
        }
        break;
    }
    case HID_REPORT_TYPE_OUTPUT:
    {
        ps4_output_report *report = (ps4_output_report *)buffer;
        set_rumble(report->motor_left, report->motor_right);
        set_lightbar(report->lightbar_red, report->lightbar_green, report->lightbar_blue);
        for (int i = 0; i < 4; i++)
        {
            if (report->lightbar_red == ps4_colors[i][0] && report->lightbar_green == ps4_colors[i][1] && report->lightbar_blue == ps4_colors[i][2])
            {
                set_player_led(i + 1);
            }
        }
        break;
    }
    default:
        break;
    }
}