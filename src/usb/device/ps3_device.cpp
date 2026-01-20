#include "usb/device/ps3_device.h"
#include "protocols/ps4.hpp"
#include "hid_reports.h"
#include "enums.pb.h"

uint8_t ef_byte = 0;
uint8_t master_bd_addr[6];
uint8_t f5_state = 0;
const uint8_t ps3_feature_01[] = {
    0x00, 0x01, 0x04, 0x00, 0x07, 0x0c, 0x01, 0x02,
    0x18, 0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11,
    0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t ps3_feature_f2[] = {
    0xf2, 0xff, 0xff, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // device bdaddr
    0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t ps3_feature_f5[] = {
    0x01, 0x00,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, // dummy PS3 bdaddr
    0xff, 0xf7, 0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t ps3_feature_f7[] = {
    0x01, 0x04, 0xc4, 0x02, 0xd6, 0x01, 0xee, 0xff,
    0x14, 0x13, 0x01, 0x02, 0xc4, 0x01, 0xd6, 0x00,
    0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x02, 0x62, 0x01, 0x02, 0x01,
    0x5e, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t ps3_feature_f8[] = {
    0x00, 0x01, 0x00, 0x00, 0x07, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Gyro and accel calibration are in here somewhere!
const uint8_t ps3_feature_ef[] = {
    0x00, 0xef, 0x04, 0x00, 0x05, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// PowerGig guitars send a specific response when woken up
const uint8_t powergig_response[3][8] = {{0xe9, 0x6d, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00},
                                         {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                         {0x21, 0x26, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00}};

// Pro instruments send specific reports when woken up
const uint8_t disabled_response[5][8] = {{0xe9, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0d, 0x01},
                                         {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                         {0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x82},
                                         {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                         {0x21, 0x26, 0x02, 0x06, 0x00, 0x00, 0x00, 0x00}};

const uint8_t enabled_response[5][8] = {{0xe9, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
                                        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                        {0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8a},
                                        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                        {0x21, 0x26, 0x02, 0x06, 0x00, 0x00, 0x00, 0x00}};

const uint8_t ps3_init[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};

uint8_t handle_player_leds_ds3(uint8_t player_mask)
{
    if (player_mask == 2)
    {
        return 1;
    }
    if (player_mask == 4)
    {
        return 2;
    }
    if (player_mask == 8)
    {
        return 3;
    }
    if (player_mask == 16)
    {
        return 4;
    }
    if (player_mask == 18)
    {
        return 5;
    }
    if (player_mask == 20)
    {
        return 6;
    }
    if (player_mask == 24)
    {
        return 7;
    }
    return 0;
}
uint8_t handle_player_leds_ps3(uint8_t player_mask)
{
    if (player_mask == 1)
    {
        return 1;
    }
    if (player_mask == 2)
    {
        return 2;
    }
    if (player_mask == 4)
    {
        return 3;
    }
    if (player_mask == 8)
    {
        return 4;
    }
    if (player_mask == 9)
    {
        return 5;
    }
    if (player_mask == 10)
    {
        return 6;
    }
    if (player_mask == 12)
    {
        return 7;
    }
    return 0;
}

uint8_t const desc_hid_report_ps3[] =
    {
        TUD_HID_REPORT_DESC_PS3_THIRDPARTY_GAMEPAD()};
uint8_t const desc_hid_report_ps3_gamepad[] =
    {
        TUD_HID_REPORT_DESC_PS3_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

PS3GamepadDevice::PS3GamepadDevice(bool wiirb) : m_wiirb(wiirb)
{
}
void PS3GamepadDevice::initialize()
{
}
void PS3GamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || !m_eps_assigned || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    if (subtype == Gamepad)
    {
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)epin_buf;
        report->report_id = 1;
        report->accelX = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->accelY = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->accelZ = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->gyro = __builtin_bswap16(PS3_ACCEL_CENTER);
        report->leftStickX = PS3_STICK_CENTER;
        report->leftStickY = PS3_STICK_CENTER;
        report->rightStickX = PS3_STICK_CENTER;
        report->rightStickY = PS3_STICK_CENTER;
    }
    else
    {
        PS3Dpad_Data_t *gamepad = (PS3Dpad_Data_t *)epin_buf;
        gamepad->accelX = PS3_ACCEL_CENTER;
        gamepad->accelY = PS3_ACCEL_CENTER;
        gamepad->accelZ = PS3_ACCEL_CENTER;
        gamepad->gyro = PS3_ACCEL_CENTER;
        gamepad->leftStickX = PS3_STICK_CENTER;
        gamepad->leftStickY = PS3_STICK_CENTER;
        if (subtype == ProKeys)
        {
            gamepad->rightStickX = 0;
            gamepad->rightStickY = 0;
        }
        else
        {
            gamepad->rightStickX = PS3_STICK_CENTER;
            gamepad->rightStickY = PS3_STICK_CENTER;
        }
    }
    for (const auto &mapping : mappings)
    {
        mapping->update(full_poll);
        mapping->update_ps3(epin_buf);
    }

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    if (subtype == Gamepad)
    {
        usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(PS3Gamepad_Data_t));
    }
    else
    {
        // convert bitmask dpad to actual hid dpad
        PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)epin_buf;
        report->dpad = GamepadButtonMapping::dpad_bindings[report->dpad];
        if (subtype == GuitarHeroGuitar)
        {
            // convert bitmask slider to actual hid slider
            PS3GuitarHeroGuitar_Data_t *reportGh = (PS3GuitarHeroGuitar_Data_t *)epin_buf;
            reportGh->slider = GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider];
        }
        usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(XInputGamepad_Data_t));
    }
}

size_t PS3GamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : m_interface,
        Reserved : 0x01,
        CompatibleID : "WINUSB",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    assert(sizeof(section) <= remaining);
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t PS3GamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epin();
    }
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(m_interface, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps3), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void PS3GamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x0c70;
    desc->idProduct = 0x0777;
}
const uint8_t *PS3GamepadDevice::report_descriptor()
{
    return desc_hid_report_ps3;
}

uint16_t PS3GamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_ps3);
}

uint16_t PS3GamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    printf("get report: %02x %02x %04x\r\n", report_id, report_type, reqlen);
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    if (report_type != HID_REPORT_TYPE_FEATURE)
    {
        return 0;
    }
    switch (report_id)
    {
    case ReportId::ReportIdPs3F2:
        memcpy(buffer, ps3_feature_f2, sizeof(ps3_feature_f2));
        return sizeof(ps3_feature_f2);

    case ReportId::ReportIdPs3F8:
        memcpy(buffer, ps3_feature_f8, sizeof(ps3_feature_f8));
        buffer[7] = ef_byte;
        return sizeof(ps3_feature_f8);

    case ReportId::ReportIdPs3F7:
        memcpy(buffer, ps3_feature_f7, sizeof(ps3_feature_f7));
        return sizeof(ps3_feature_f7);
    case ReportId::ReportIdPs3F5:
        memcpy(buffer, ps3_feature_f5, sizeof(ps3_feature_f5));
        if (f5_state == 0)
        {
            /*
             * First request, tell that the bdaddr is not the one of the PS3.
             */
            f5_state = 1;
        }
        else
        {
            /*
             * Next requests, tell that the bdaddr is the one of the PS3.
             */
            memcpy(buffer + 2, master_bd_addr, sizeof(master_bd_addr));
        }
        return sizeof(ps3_feature_f5);
    case ReportId::ReportIdPs3EF:
        memcpy(buffer, ps3_feature_ef, sizeof(ps3_feature_ef));
        buffer[7] = ef_byte;
        return sizeof(ps3_feature_ef);
    case ReportId::ReportIdPs301:
        // PS3 requests this for a gamepad
        memcpy(buffer, ps3_feature_01, sizeof(ps3_feature_01));
        return sizeof(ps3_feature_01);
    case ReportId::ReportIdPs3Feature:
        memcpy(buffer, ps3_init, sizeof(ps3_init));
        switch (subtype)
        {
        case GuitarHeroGuitar:
        case RockBandGuitar:
        case LiveGuitar:
        case DjHeroTurntable:
            buffer[4] = 0x06;
            break;
        case RockBandDrums:
            buffer[4] = 0x05;
            break;
        case ProGuitarMustang:
        case ProGuitarSquire:
        case ProKeys:
            // RB Pro instruments follow a slightly different request flow
            if (m_enabled)
            {
                memcpy(buffer, enabled_response[m_pro_id], sizeof(enabled_response[m_pro_id]));
            }
            else
            {
                memcpy(buffer, disabled_response[m_pro_id], sizeof(disabled_response[m_pro_id]));
            }
            m_pro_id++;
            if (m_pro_id > 4)
                m_pro_id = 4;
            break;
            // as does power gig
        case PowerGigGuitar:
        case PowerGigDrum:
            memcpy(buffer, powergig_response[m_pg_id], sizeof(powergig_response[m_pg_id]));
            m_pg_id++;
            if (m_pg_id > 2)
                m_pg_id = 2;
            break;
        default:
            break;
        }
        return sizeof(ps3_init);
    }
    // TODO: need to do the ps3 feature report
    return 0;
}
void PS3GamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
        switch (report_id)
        {
        case ReportId::ReportIdPs3Feature:
            switch (subtype)
            {
            case ProGuitarMustang:
            case ProGuitarSquire:
            case ProKeys:
                if (buffer[0] == 0xe9 && buffer[2] == 0x89) {
                    m_enabled = true;
                    m_pro_id = 0;
                }
                if (buffer[0] == 0xe9 && buffer[2] == 0x81) {
                    m_enabled = false;
                    m_pro_id = 0;
                }
                break;
            case PowerGigGuitar:
            case PowerGigDrum:
                if (buffer[0] == 0xe9 && buffer[0] == 0x4d) {
                    m_enabled = true;
                    m_pg_id = 0;
                }
            default:
                break;
            }
            return;
        case ReportId::ReportIdPs3EF:
            ef_byte = buffer[6];
            return;
        }
        break;
    case HID_REPORT_TYPE_OUTPUT:
    {
        // rumble / led packets will be here

        uint8_t id = buffer[0];
        if (id == PS3_LED_ID)
        {
            player_led = handle_player_leds_ds3(buffer[9]);
            rumble_left = buffer[0x04];
            rumble_right = buffer[0x02] ? 0xff : 0;
        }
        else if (id == PS3_RUMBLE_ID)
        {
            // instruments receive player leds over this report id with one format
            if (bufsize >= 8)
            {
                uint8_t player = buffer[3];
                player_led = handle_player_leds_ps3(player);
            }
            else
            {
                // and DS3s receive rumble, and the packet length is shorter
                rumble_left = buffer[0x05];
                rumble_right = buffer[0x03] ? 0xff : 0;
            }
        }
        else if (id == DJ_LED_ID)
        {
            uint8_t euphoria_on = buffer[2] * 0xFF;
            euphoria_led = euphoria_on;
        }
        else if (id == SANTROLLER_LED_ID)
        {
            stagekit_command = buffer[3];
            stagekit_param = buffer[2];
        }
        break;
    }
    default:
        break;
    }
}