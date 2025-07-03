#include <Arduino.h>
#include <SPI.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <sleep.h>
#include <string.h>
#include <tusb.h>
#include <EEPROM.h>

#include "commands.h"
#include "common/tusb_types.h"
#include "config.h"
#include "controllers.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"
#include "fxpt_math.h"
#include "hardware/structs/usb.h"
#include "hardware/watchdog.h"
#include "hid.h"
#include "hidescriptorparser.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "io.h"
#include "midi_host.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pin_funcs.h"
#include "reports/controller_reports.h"
#include "serial.h"
#include "shared_main.h"
#include "xinput_device.h"
#include "xinput_host.h"
#if defined(INPUT_USB_HOST) || defined(INPUT_SERIAL_MIDI)
#include "TUSB-MIDI.hpp"
#endif
#if BLUETOOTH
#include "bt.h"
#include "pico/cyw43_arch.h"
#endif

CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN STRING_DESCRIPTOR_PICO serialstring = {
    .bLength = (sizeof(uint8_t) + sizeof(uint8_t) + (SERIAL_LEN + 3) * 2),
    .bDescriptorType = USB_DESCRIPTOR_STRING,
    .UnicodeString = {}};
#define PERSISTED_CONSOLE_TYPE_VALID 0x3A2F
static uint32_t __uninitialized_ram(persistedConsoleType);
static uint32_t __uninitialized_ram(windows_in_hid);
static uint32_t __uninitialized_ram(persistedConsoleTypeValid);
static uint32_t __uninitialized_ram(xboxAuthValid);
static uint32_t __uninitialized_ram(pico_is_sleeping);
#if USB_HOST_STACK
USB_Device_Type_t xone_dev_addr = {};
USB_Device_Type_t x360_dev_addr = {};
USB_Device_Type_t ps4_dev_addr = {};
uint8_t total_usb_host_devices = 0;
typedef struct
{
    USB_Device_Type_t type;
    USB_LastReport_Data_t report;
    uint8_t report_length;
    bool switch_sent_timeout;
    bool switch_sent_handshake;
    uint8_t xone_init_id;
} Usb_Host_Device_t;

Usb_Host_Device_t usb_host_devices[CFG_TUH_DEVICE_MAX * CFG_TUH_XINPUT];
#endif
typedef enum
{
    PIO_USB_PINOUT_DPDM = 0, // DM = DP+1
    PIO_USB_PINOUT_DMDP,     // DM = DP-1
} PIO_USB_PINOUT;
typedef struct
{
    uint8_t pin_dp;
    uint8_t pio_tx_num;
    uint8_t sm_tx;
    uint8_t tx_ch;
    uint8_t pio_rx_num;
    uint8_t sm_rx;
    uint8_t sm_eop;
    void *alarm_pool;
    int8_t debug_pin_rx;
    int8_t debug_pin_eop;
    bool skip_alarm_pool;
    PIO_USB_PINOUT pinout;
} pio_usb_configuration_t;
uint8_t prev_bt_report[32];
static const uint8_t capabilitiesRequest[] = {0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_prescence[] = {0x08, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if defined(INPUT_USB_HOST) || defined(INPUT_SERIAL_MIDI)
using namespace usbMidi;
#endif
#ifdef INPUT_USB_HOST
UsbMidiTransport usbMIDITransport(0);
MidiInterface<UsbMidiTransport> MIDI(usbMIDITransport);
#endif
#ifdef INPUT_SERIAL_MIDI
#if SERIAL_MIDI_PIN == 5 || SERIAL_MIDI_PIN == 9 || SERIAL_MIDI_PIN == 20
SerialMIDI<HardwareSerial> serialMIDI(Serial2);
#else
SerialMIDI<HardwareSerial> serialMIDI(Serial1);
#endif
MidiInterface<SerialMIDI<HardwareSerial>> MIDI2(serialMIDI);
#endif
bool ready_for_next_packet()
{
    return tud_xinput_n_ready(0) && tud_ready_for_packet();
}

bool usb_configured()
{
    return tud_ready();
}

void send_report_to_pc(const void *report, uint8_t len)
{
    tud_xusb_n_report(0, report, len);
}
bool foundXB = false;
bool authReady = false;
bool authDone = false;
long test3 = 0;
static void tick_usb()
{
#if USB_HOST_STACK
    if (consoleType == XBOXONE && !foundXB)
    {
        tuh_task();
        return;
    }
#endif
    tud_task();
#if USB_HOST_STACK
    tuh_task();
#endif
    tick();
#ifdef INPUT_USB_HOST
    MIDI.read();
#endif
#ifdef INPUT_SERIAL_MIDI
    MIDI2.read();
#endif
}
#if BLUETOOTH_RX
void tick_bluetooth(const void *buf, uint8_t len, USB_Device_Type_t type)
{
    tick_bluetooth_inputs(buf, len, type);
}
#endif
#if BLUETOOTH
void loop1()
{
}
void setup1()
{
    btstack_main();
}
#endif
void loop()
{
    tick_usb();
}
void setArcadeSide(uint8_t side)
{
    if (arcadeSide == side)
    {
        return;
    }
    arcadeSide = side;
    EEPROM.write(0, side);
    EEPROM.commit();
}

void go_to_sleep()
{
    pico_is_sleeping = true;
    reset_usb();
}

void wakeup_360()
{
    if (tud_suspended())
    {
        tud_remote_wakeup();
    }
    if (usb_configured())
    {
        return;
    }
    // the 360 will wake up the console if it sees the data lines being driven high and low constantly
    // save the previous usb state
    uint32_t prev_direct = usb_hw->phy_direct;
    uint32_t prev_direct_override = usb_hw->phy_direct_override;
    for (int i = 0; i < 2; i++)
    {
        // drive both pins high
        hw_set_bits(&usb_hw->phy_direct_override, USB_USBPHY_DIRECT_OVERRIDE_TX_DM_OE_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_DM_OVERRIDE_EN_BITS |
                                                      USB_USBPHY_DIRECT_OVERRIDE_TX_DP_OE_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_DP_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_DIFFMODE_OVERRIDE_EN_BITS);
        hw_set_bits(&usb_hw->phy_direct, USB_USBPHY_DIRECT_TX_DM_OE_BITS | USB_USBPHY_DIRECT_TX_DP_OE_BITS | USB_USBPHY_DIRECT_TX_DIFFMODE_BITS);
        hw_clear_bits(&usb_hw->phy_direct, USB_USBPHY_DIRECT_TX_DIFFMODE_BITS);
        hw_set_bits(&usb_hw->phy_direct, USB_USBPHY_DIRECT_TX_DP_BITS | USB_USBPHY_DIRECT_TX_DM_BITS);
        // wait a bit
        sleep_ms(1);
        // revert the pin state
        usb_hw->phy_direct = prev_direct;
        usb_hw->phy_direct_override = prev_direct_override;
        sleep_ms(1000);
    }
    // Reset the pico just to make sure any other state is reset in case this was triggered by usb host
    reset_usb();
}

void setup()
{
#if USB_HOST_STACK
    set_sys_clock_khz(120000, true);
#endif
#ifdef INPUT_SERIAL_MIDI
#if SERIAL_MIDI_PIN == 5 || SERIAL_MIDI_PIN == 9 || SERIAL_MIDI_PIN == 20
    Serial2.setRX(SERIAL_MIDI_PIN);
#else
    Serial1.setRX(SERIAL_MIDI_PIN);
#endif
#endif
    if (persistedConsoleTypeValid == PERSISTED_CONSOLE_TYPE_VALID)
    {
        consoleType = persistedConsoleType;
        if (SLEEP_PIN != -1)
        {
            // Sleep works best when nothing else is started, so we reboot the pico before and after sleep
            if (pico_is_sleeping)
            {
                pinMode(SLEEP_PIN, SLEEP_ACTIVE_HIGH ? INPUT_PULLDOWN : INPUT_PULLUP);
                sleep_run_from_xosc();
                sleep_goto_dormant_until_pin(SLEEP_PIN, true, SLEEP_ACTIVE_HIGH);
                sleep_power_up();
                pico_is_sleeping = false;
                reset_usb();
            }
        }
    }
    else
    {
        windows_in_hid = false;
        xboxAuthValid = false;
        consoleType = UNIVERSAL;
        pico_is_sleeping = false;
    }
    generateSerialString(&serialstring, consoleType);
    printf("ConsoleType: %d\r\n", consoleType);

    EEPROM.begin(512);
    arcadeSide = EEPROM.read(0);
    if (arcadeSide > 2)
    {
        arcadeSide = 1;
    }
    init_main();
    tud_init(TUD_OPT_RHPORT);
#if USB_HOST_STACK
    pio_usb_configuration_t config = {
        pin_dp : USB_HOST_DP_PIN,
        pio_tx_num : 0,
        sm_tx : 0,
        tx_ch : 0,
        pio_rx_num : 0,
        sm_rx : 1,
        sm_eop : 2,
        alarm_pool : NULL,
        debug_pin_rx : -1,
        debug_pin_eop : -1,
        skip_alarm_pool : false,
        pinout : PIO_USB_PINOUT_DPDM
    };
    tuh_configure(0, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config);
    tuh_init(TUH_OPT_RHPORT);
#ifdef INPUT_USB_HOST
    MIDI.begin(0);
    MIDI.setHandleNoteOn(onNote);
    MIDI.setHandleNoteOff(offNote);
    MIDI.setHandleControlChange(onControlChange);
    MIDI.setHandlePitchBend(onPitchBend);
#endif
#ifdef INPUT_SERIAL_MIDI
    MIDI2.begin(0);
    MIDI2.setHandleNoteOn(onNote);
    MIDI2.setHandleNoteOff(offNote);
    MIDI2.setHandleControlChange(onControlChange);
    MIDI2.setHandlePitchBend(onPitchBend);
#endif
#endif
}

#ifdef INPUT_USB_HOST
void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets)
{
    usbMIDITransport.tuh_midi_rx_cb(dev_addr, num_packets);
}

void tuh_midi_mount_cb(uint8_t dev_addr, uint8_t in_ep, uint8_t out_ep, uint8_t num_cables_rx, uint16_t num_cables_tx)
{
    printf("MIDI device address = %u, IN endpoint %u has %u cables, OUT endpoint %u has %u cables\r\n",
           dev_addr, in_ep & 0xf, num_cables_rx, out_ep & 0xf, num_cables_tx);

    usbMIDITransport.midi_dev_addr = dev_addr;

    USB_Device_Type_t type = {MIDI_ID, 0, dev_addr, 0};
    usb_host_devices[total_usb_host_devices].type = type;
    total_usb_host_devices++;
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    printf("MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);

    usbMIDITransport.midi_dev_addr = 0;
    // Probably should actulaly work out what was unplugged and all that
    total_usb_host_devices = 0;
}
#endif
void authentication_successful()
{
    printf("Auth done\r\n");
    authReady = true;
}

#if USB_HOST_STACK
USB_Device_Type_t get_device_address_for(uint8_t deviceType)
{
    if (deviceType == XBOXONE)
    {
        return xone_dev_addr;
    }
    if (deviceType == XBOX360)
    {
        return x360_dev_addr;
    }
    if (deviceType == PS4)
    {
        return ps4_dev_addr;
    }
    return {0};
}
#endif
void send_report_to_controller(uint8_t dev_addr, uint8_t instance, const uint8_t *report, uint8_t len)
{
    if (dev_addr && tuh_xinput_mounted(dev_addr, instance))
    {
        tuh_xinput_send_report(dev_addr, instance, report, len);
    }
}
extern "C" void tud_mount_cb(void)
{
    device_reset();
}
#if USB_HOST_STACK
uint8_t get_usb_host_device_count()
{
    return total_usb_host_devices;
}
USB_Device_Type_t get_usb_host_device_type(uint8_t id)
{
    return usb_host_devices[id].type;
}

uint8_t get_usb_host_device_data(uint8_t id, uint8_t *buf)
{
    if (usb_host_devices[id].type.console_type == GENERIC)
    {
        USB_Host_Data_t *host = (USB_Host_Data_t *)buf;
        fill_generic_report_host(usb_host_devices[id].type.dev_addr, usb_host_devices[id].type.instance, (uint8_t *)&usb_host_devices[id].report, host);

        // for (int i = 20; i < sizeof(USB_Host_Data_t); i++) {
        //     printf("%02x, ", buf[i]);
        // }
        // printf("\r\n");
        return sizeof(USB_Host_Data_t);
    }
    memcpy(buf, &usb_host_devices[id].report, usb_host_devices[id].report_length);
    return usb_host_devices[id].report_length;
}

uint8_t read_usb_host_devices(uint8_t *buf)
{
    for (int i = 0; i < total_usb_host_devices; i++)
    {
        USB_Device_Type_t *type = &usb_host_devices[i].type;
        buf[(i * 2)] = type->console_type;
        buf[(i * 2) + 1] = type->sub_type;
    }
    return total_usb_host_devices * 2;
}

bool tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t console_type, uint8_t sub_type)
{
    printf("Detected controller: %d (%d) on %d, %d\r\n", console_type, sub_type, dev_addr, instance);
    uint16_t host_vid = 0;
    uint16_t host_pid = 0;
    tuh_vid_pid_get(dev_addr, &host_vid, &host_pid);
    printf("VID: %04x, PID: %04x \r\n", host_vid, host_pid);
    USB_Device_Type_t type = {console_type, sub_type, dev_addr, instance, DRUM_UNKNOWN};
    tuh_descriptor_get_device_sync(dev_addr, buf, sizeof(USB_DEVICE_DESCRIPTOR));
    USB_DEVICE_DESCRIPTOR *desc = (USB_DEVICE_DESCRIPTOR *)buf;
    get_usb_device_type_for(host_vid, host_pid, desc->bcdDevice, &type);
    switch (type.console_type)
    {
    case XBOX360:
        x360_dev_addr = type;
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        xinput_controller_connected(host_vid, host_pid);
        break;
    case XBOX360_W:
        x360_dev_addr = type;
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        printf("found xb360 wireless\r\n");
        send_report_to_controller(dev_addr, instance, xbox360w_prescence, sizeof(xbox360w_prescence));
        break;
    case XBOXONE:
        xone_dev_addr = type;
        xone_controller_connected(dev_addr, instance);
        usb_host_devices[total_usb_host_devices].type = type;
        usb_host_devices[total_usb_host_devices].xone_init_id = 0;
        total_usb_host_devices++;
        if (consoleType == XBOXONE)
        {
            foundXB = true;
        }
        break;
    case SANTROLLER:
    {
        tuh_descriptor_get_product_string_sync(dev_addr, 0, buf, sizeof(buf));
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        printf("Found Santroller controller\r\n");
        printf("Sub type: %d\r\n", type.sub_type);
        break;
    }
    case RAPHNET:
    {
        uint8_t data[] = {0x00, 0x00, 0x00};
        uint8_t data2[] = {0x06, 0x00, 0x00};
        for (int i = 0; i < 10; i++)
        {
            transfer_with_usb_controller(dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE, HID_REQUEST_SET_REPORT, 0x0300, 1, 3, data2, NULL);
            transfer_with_usb_controller(dev_addr, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE, HID_REQUEST_GET_REPORT, 0x0300, 1, 3, data, NULL);
            if (data[0])
            {
                break;
            }
            sleep_ms(100);
        }
        switch (data[2])
        {
        case RNT_TYPE_PSX_DIGITAL:
        case RNT_TYPE_PSX_ANALOG:
        case RNT_TYPE_PSX_NEGCON:
        case RNT_TYPE_PSX_MOUSE:
        case RNT_TYPE_CLASSIC:
        case RNT_TYPE_UDRAW_TABLET:
        case RNT_TYPE_NUNCHUK:
        case RNT_TYPE_CLASSIC_PRO:
            type.sub_type = GAMEPAD;
            break;
        case RNT_TYPE_WII_GUITAR:
            type.sub_type = GUITAR_HERO_GUITAR;
            break;
        case RNT_TYPE_WII_DRUM:
            type.sub_type = GUITAR_HERO_DRUMS;
            break;
        }
        printf("Found Raphnet controller\r\n");
        printf("Sub type: %02x\r\n", type.sub_type);
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        break;
    }
    case XBOX360_BB:
    case OG_XBOX:
    case KEYBOARD:
    case MOUSE:
    case STREAM_DECK:
    case PS5:
    case SWITCH:
    case UNKNOWN:
    case GENERIC:
    case STEPMANIAX:
    case LTEK:
    case LTEK_ID:
    case XINPUTCOMPAT:
    case STADIA:
        printf("Found Generic controller\r\n");
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        break;
    case SWITCH2:
        printf("Found Switch 2 controller\r\n");
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        if (type.sub_type == NON_CONTROLLER)
        {
            uint8_t buf[] = {0x03, 0x91, 0x00, 0x0d, 0x00, 0x08,
                             0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            send_report_to_controller(dev_addr, instance, buf, sizeof(buf));
        }
        break;

    case PS3:
        // GHWT and GH5 guitars have the same vid and pid, but different tap bar functions. We can read the device name to actually determine what it is
        if (type.sub_type == GUITAR_HERO_GUITAR && XFER_RESULT_SUCCESS == tuh_descriptor_get_product_string_sync(dev_addr, 0, buf, sizeof(buf)))
        {
            uint16_t wtProduct[] = {'G', 'u', 'i', 't', 'a', 'r', ' ', 'H', 'e', 'r', 'o', '4'};
            if (memcmp(wtProduct, buf, sizeof(wtProduct)) || memcmp(wtProduct, buf + 1, sizeof(wtProduct)))
            {
                type.sub_type = GUITAR_HERO_GUITAR_WT;
            }
        }
        usb_host_devices[total_usb_host_devices].type = type;
        usb_host_devices[total_usb_host_devices].xone_init_id = millis();
        total_usb_host_devices++;
        printf("Found PS3 controller\r\n");
        printf("Sub type: %d\r\n", type.sub_type);
        ps3_controller_connected(dev_addr, host_vid, host_pid);
        break;
    case PS4:
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        if (!ps4_dev_addr.dev_addr)
        {
            ps4_dev_addr = type;

            printf("Found PS4 controller\r\n");
            ps4_controller_connected(dev_addr, host_vid, host_pid);
        }
        break;
    }
    printf("Total devices: %d\r\n", total_usb_host_devices);

    host_controller_connected();
    return true;
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    printf("Unplugged %d\r\n", dev_addr);
    if (xone_dev_addr.dev_addr == dev_addr && xone_dev_addr.instance == instance)
    {
        xone_dev_addr.dev_addr = 0;
    }
    if (x360_dev_addr.dev_addr == dev_addr && x360_dev_addr.instance == instance)
    {
        x360_dev_addr.dev_addr = 0;
    }
    if (ps4_dev_addr.dev_addr == dev_addr && ps4_dev_addr.instance == instance)
    {
        ps4_dev_addr.dev_addr = 0;
        ps4_controller_disconnected();
    }
    // Probably should actulaly work out what was unplugged and all that
    total_usb_host_devices = 0;
}
bool wasXb1Input = false;
void tuh_xinput_report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
    // If there are xb1 init packets to send, send them
    for (int i = 0; i < total_usb_host_devices; i++)
    {
        if (usb_host_devices[i].type.dev_addr == dev_addr && usb_host_devices[i].type.instance == instance)
        {
            if (usb_host_devices[i].type.console_type == XBOXONE)
            {
                if (xone_controller_send_init_packet(dev_addr, instance, usb_host_devices[i].xone_init_id))
                {
                    usb_host_devices[i].xone_init_id++;
                }
            }
            if (usb_host_devices[i].type.console_type == SWITCH2 && usb_host_devices[i].type.sub_type == NON_CONTROLLER)
            {
                if (!usb_host_devices[i].switch_sent_timeout)
                {
                    usb_host_devices[i].switch_sent_timeout = true;
                    uint8_t buf[] = {0x09, 0x91, 0x00, 0x07, 0x00, 0x08, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0, 0};
                    send_report_to_controller(dev_addr, instance, buf, sizeof(buf));
                }
            }
        }
    }
}
long lastPlayerLed = 0;
long lastWakeup = 0;
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
    if (!len)
    {
        // Skip processing a null report, assume it is corrupted
        return;
    }
    if (dev_addr == xone_dev_addr.dev_addr && instance == xone_dev_addr.instance)
    {
        receive_report_from_controller(report, len);
    }
    // Send player indicator every 10 seconds or so just as a keep alive
    if (millis() - lastPlayerLed > 10000)
    {
        handle_player_leds(0);
        lastPlayerLed = millis();
    }
    for (int i = 0; i < total_usb_host_devices; i++)
    {
        if (usb_host_devices[i].type.dev_addr == dev_addr && usb_host_devices[i].type.instance == instance)
        {
            if (usb_host_devices[i].type.console_type == XBOXONE)
            {
                GipHeader_t *header = (GipHeader_t *)report;
                if (header->command == GIP_VIRTUAL_KEYCODE)
                {
                    GipKeystroke_t *keystroke = (GipKeystroke_t *)report;
                    if (wasXb1Input)
                    {
                        XboxOneInputHeader_Data_t *gamepad = (XboxOneInputHeader_Data_t *)(&(usb_host_devices[i].report));
                        gamepad->guide = keystroke->pressed;
                    }
                    return;
                }
                if (header->command != GHL_HID_REPORT && header->command != GIP_INPUT_REPORT)
                {
                    return;
                }
                wasXb1Input = header->command == GIP_INPUT_REPORT;
            }
            if ((millis() - usb_host_devices[i].xone_init_id) < 5000)
            {
                if (usb_host_devices[i].type.console_type == PS3 && usb_host_devices[i].type.sub_type == ROCK_BAND_PRO_KEYS)
                {
                    uint8_t hid_command_enable[40] = {
                        0xE9, 0x00, 0x89, 0x1B, 0x00, 0x00, 0x00, 0x02,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
                        0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0xE9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                    transfer_with_usb_controller(dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0300, 0, sizeof(hid_command_enable), hid_command_enable, NULL);
                }
            }
            if (usb_host_devices[i].type.console_type == XBOX360_W)
            {
                // Corrupt report, ignore
                if (len < sizeof(XBOX_WIRELESS_HEADER))
                {
                    return;
                }
                XBOX_WIRELESS_HEADER *header = (XBOX_WIRELESS_HEADER *)report;
                if (header->id == 0x08)
                {
                    // Disconnected
                    if (header->type == 0x00)
                    {
                        usb_host_devices[i].type.sub_type = UNKNOWN;
                        printf("Disconnected\r\n", dev_addr, instance);
                    }
                }
                else if (header->id == 0x00)
                {
                    // Gamepad inputs
                    if (header->type == 0x01 || header->type == 0x03)
                    {
                        memcpy(&usb_host_devices[i].report, report + sizeof(header), len - sizeof(header));
                        usb_host_devices[i].report_length = len - sizeof(header);
                        if (usb_host_devices[i].type.sub_type == XINPUT_DRUMS && usb_host_devices[i].type.drum_type == DRUM_UNKNOWN)
                        {
                            XInputGamepad_Data_t *gamepad = (XInputGamepad_Data_t *)&usb_host_devices[i].report;
                            if (gamepad->leftThumbClick)
                            {
                                usb_host_devices[i].type.drum_type = DRUM_GH;
                            }
                            else
                            {
                                // Only RB2/3 kits are wireless
                                usb_host_devices[i].type.drum_type = DRUM_RB2;
                            }
                        }
                        if (usb_host_devices[i].type.sub_type == XINPUT_GUITAR_ALTERNATE && usb_host_devices[i].type.drum_type == GUITAR_CHECK)
                        {
                            // request capabilities
                            send_report_to_controller(dev_addr, instance, capabilitiesRequest, sizeof(capabilitiesRequest));
                            usb_host_devices[i].type.drum_type == DRUM_UNKNOWN;
                        }
                    }
                    // Link report
                    if (header->type == 0x0f)
                    {
                        XBOX_WIRELESS_LINK_REPORT *linkReport = (XBOX_WIRELESS_LINK_REPORT *)report;
                        if (linkReport->always_0xCC != 0xCC)
                        {
                            return;
                        }
                        uint8_t sub_type = linkReport->subtype & ~0x80;
                        usb_host_devices[i].type.sub_type = sub_type;
                        printf("Found subtype: %02x %02x %02x\r\n", sub_type, dev_addr, instance);
                        xinput_w_controller_connected();
                        // Request capabilities so we can figure out WT guitars
                        if (sub_type == XINPUT_GUITAR_ALTERNATE)
                        {
                            usb_host_devices[i].type.drum_type = GUITAR_CHECK;
                        }
                    }
                    // Capabilities report
                    if (header->type == 0x05)
                    {
                        XBOX_WIRELESS_CAPABILITIES *caps = (XBOX_WIRELESS_CAPABILITIES *)report;
                        if (caps->always_0x12 != 0x12)
                        {
                            return;
                        }
                        printf("Found capabilities: %02x %02x\r\n", dev_addr, instance);
                        if (caps->leftStickX == 0xFFC0 && caps->rightStickX == 0xFFC0)
                        {
                            usb_host_devices[i].type.sub_type = XINPUT_GUITAR_WT;
                            printf("Found wt\r\n");
                        }
                    }
                }
                return;
            }
            if (usb_host_devices[i].type.console_type == STREAM_DECK && report[0] != STREAM_DECK_INPUT_REPORT_ID)
            {
                continue;
            }
            if (usb_host_devices[i].type.console_type == PS5 && report[0] != PS5_INPUT_REPORT_ID)
            {
                continue;
            }
            if (usb_host_devices[i].type.console_type == STEPMANIAX && report[0] != STEPMANIA_X_REPORT_ID)
            {
                continue;
            }
            if (usb_host_devices[i].type.console_type == STADIA && report[0] != STADIA_REPORT_ID)
            {
                continue;
            }
            if (usb_host_devices[i].type.console_type == SWITCH && usb_host_devices[i].type.sub_type == GAMEPAD)
            {
                if (!usb_host_devices[i].switch_sent_handshake)
                {
                    usb_host_devices[i].switch_sent_handshake = true;
                    uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
                    send_report_to_controller(dev_addr, instance, buf, 2);
                }
                else if (!usb_host_devices[i].switch_sent_timeout)
                {
                    usb_host_devices[i].switch_sent_timeout = true;
                    uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x04 /* PROCON_USB_ENABLE */};
                    send_report_to_controller(dev_addr, instance, buf, 2);
                }

                if (report[0] != SWITCH_PRO_CON_FULL_REPORT_ID)
                {
                    continue;
                }
            }
            if (usb_host_devices[i].type.console_type == SWITCH2 && usb_host_devices[i].type.sub_type == GAMEPAD)
            {
                if (report[0] != SWITCH_2_GC_FULL_REPORT_ID)
                {
                    continue;
                }
            }
            if (usb_host_devices[i].type.console_type == XBOX360)
            {

                XInputGamepad_Data_t *gamepad = (XInputGamepad_Data_t *)&usb_host_devices[i].report;
                if (len != sizeof(XInputGamepad_Data_t))
                {
                    return;
                }
            }

            memcpy(&usb_host_devices[i].report, report, len);
            usb_host_devices[i].report_length = len;
            // Xinput drum detection
            if (usb_host_devices[i].type.console_type == XBOX360 && usb_host_devices[i].type.sub_type == XINPUT_DRUMS && usb_host_devices[i].type.drum_type == DRUM_UNKNOWN)
            {
                XInputRockBandDrums_Data_t *drums = (XInputRockBandDrums_Data_t *)&usb_host_devices[i].report;
                if (drums->leftThumbClick)
                {
                    usb_host_devices[i].type.drum_type = DRUM_GH;
                }
                else
                {
                    // Only RB1 kits are wired
                    usb_host_devices[i].type.drum_type = DRUM_RB1;
                }
            }
            return;
        }
    }
}

usbh_class_driver_t driver_host[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Host_HID",
#endif
        .init = xinputh_init,
        .open = xinputh_open,
        .set_config = xinputh_set_config,
        .xfer_cb = xinputh_xfer_cb,
        .close = xinputh_close},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "MIDI_Host_HID",
#endif
        .init = midih_init,
        .open = midih_open,
        .set_config = midih_set_config,
        .xfer_cb = midih_xfer_cb,
        .close = midih_close}};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = 2;
    return driver_host;
}
#endif

uint8_t const *tud_descriptor_device_cb(void)
{
    descriptorRequest(USB_DESCRIPTOR_DEVICE << 8, 0, buf);
    return buf;
}
uint8_t const *tud_hid_custom_descriptor_report_cb(uint8_t instance)
{
    descriptorRequest(HID_DESCRIPTOR_REPORT << 8, instance, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; // for multiple configurations
    descriptorRequest(USB_DESCRIPTOR_CONFIGURATION << 8, index, buf);
    return buf;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3)
    {
        return (uint16_t *)&serialstring;
    }
    if (descriptorRequest(USB_DESCRIPTOR_STRING << 8 | index, 0, buf))
    {
        return (uint16_t *)buf;
    }
    return NULL;
}

uint32_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer, bool *status)
{
    if (!dev_addr)
    {
        // Device is not connected yet!
        if (status)
        {
            *status = false;
        }
        return 0;
    }
    tusb_control_request_t setup = {
        bmRequestType : requestType,
        bRequest : request,
        wValue : wValue,
        wIndex : wIndex,
        wLength : wLength
    };
    tuh_xfer_t xfer = {};
    xfer.daddr = dev_addr;
    xfer.ep_addr = 0;
    xfer.setup = &setup;
    xfer.buffer = buffer;
    xfer.complete_cb = NULL;
    xfer.user_data = 0;
    tuh_control_xfer(&xfer);
    if (xfer.result != XFER_RESULT_SUCCESS)
    {
        if (status)
        {
            *status = false;
        }
        return false;
    }
    return xfer.actual_len;
}
tusb_control_request_t lastreq;
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD && request->bRequest == TUSB_REQ_GET_DESCRIPTOR)
    {
        //------------- STD Request -------------//
        if (stage == CONTROL_STAGE_SETUP)
        {
            uint8_t const desc_type = tu_u16_high(request->wValue);
            // uint8_t const desc_index = tu_u16_low (request->wValue);

            if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_HID)
            {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_HID << 8, request->wIndex, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            }
            else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_REPORT)
            {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_REPORT << 8, request->wIndex, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            }
            else
            {
                return false; // stall unsupported request
            }
        }
    }
    if (controlRequestValid(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength))
    {
        bool status = true;
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
        {
            if (stage == CONTROL_STAGE_SETUP)
            {
                uint16_t len = controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf, &status);
                tud_control_xfer(rhport, request, buf, len);
            }
        }
        else
        {
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(rhport, request, buf, request->wLength);
            }
            if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength))
            {
                uint8_t ret = controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf, &status);
            }
        }
        if (!status)
        {
            return status;
        }
    }

    return true;
}

usbd_class_driver_t driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Device_HID",
#endif
        .init = xinputd_init,
        .reset = xinputd_reset,
        .open = xinputd_open,
        .control_xfer_cb = tud_vendor_control_xfer_cb,
        .xfer_cb = xinputd_xfer_cb,
        .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = 1;
    return driver;
}

void reboot(void)
{
    watchdog_enable(1, false);
    for (;;)
    {
    }
}
void bootloader(void)
{
    reset_usb_boot(0, 0);
}
void reset_usb(void)
{
    persistedConsoleType = consoleType;
    persistedConsoleTypeValid = PERSISTED_CONSOLE_TYPE_VALID;
    reboot();
}