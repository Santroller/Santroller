#include "bt.h"
#include "config.h"

#define MAX_ATTRIBUTE_VALUE_SIZE 300

#define INQUIRY_INTERVAL 5
#define PERIPHERAL_COD ((1 << 8) | (1 << 10))
#ifdef BLUETOOTH_RX_CLASSIC

#include <inttypes.h>
#include <stdio.h>

#include "btstack.h"
#include "btstack_config.h"
#include "shared_main.h"
#ifdef CONFIGURABLE_BLOBS
static const char *remote_addr_string = (const char *)&BT_ADDR;
#else
#ifdef BT_ADDR
static const char remote_addr_string[] = BT_ADDR;
#endif
#endif

static bool has_address = false;
static bd_addr_t remote_addr;

static btstack_packet_callback_registration_t hci_event_callback_registration;

// SDP
static uint8_t hid_descriptor_storage[500];

// App
typedef enum {
    APP_IDLE,
    APP_CONNECTED
} app_state_t;

static app_state_t app_state = APP_IDLE;

#define MAX_DEVICES 20
enum DEVICE_STATE { REMOTE_NAME_REQUEST,
                    REMOTE_NAME_INQUIRED,
                    REMOTE_NAME_FETCHED };
struct device {
    bd_addr_t address;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
    uint32_t cod;
    char name_buffer[240 + 2 + SIZE_OF_BD_ADDRESS];
    enum DEVICE_STATE state;
};
struct device devices[MAX_DEVICES];
int deviceCount = 0;

enum STATE { INIT,
             ACTIVE };
enum STATE state = INIT;

static uint16_t hid_host_cid = 0;
static bool hid_host_descriptor_available = false;
static hid_protocol_mode_t hid_host_report_mode = HID_PROTOCOL_MODE_REPORT_WITH_FALLBACK_TO_BOOT;

int get_bt_address(uint8_t *addr) {
    bd_addr_t local_addr;
    gap_local_bd_addr(local_addr);
    memcpy(addr, bd_addr_to_str(local_addr), SIZE_OF_BD_ADDRESS);
    return SIZE_OF_BD_ADDRESS;
}

bool check_bluetooth_ready() {
    return app_state == APP_CONNECTED;
}

static int getDeviceIndexForAddress(bd_addr_t addr) {
    int j;
    for (j = 0; j < deviceCount; j++) {
        if (bd_addr_cmp(addr, devices[j].address) == 0) {
            return j;
        }
    }
    return -1;
}
/* @section Main application configuration
 *
 * @text In the application configuration, L2CAP and HID host are initialized, and the link policies
 * are set to allow sniff mode and role change.
 */

/* LISTING_START(PanuSetup): Panu setup */
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static void hid_host_setup(void) {
    // Initialize L2CAP
    l2cap_init();

#ifdef ENABLE_BLE
    // Initialize LE Security Manager. Needed for cross-transport key derivation
    sm_init();
#endif

    // Initialize HID Host
    hid_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    hid_host_register_packet_handler(packet_handler);

    // Allow sniff mode requests by HID device and support role switch
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);

    // try to become master on incoming connections
    hci_set_master_slave_policy(HCI_ROLE_MASTER);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Disable stdout buffering
    setvbuf(stdin, NULL, _IONBF, 0);
}

void bt_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id) {
    hid_host_send_set_report(hid_host_cid, (hid_report_type_t)reportType, report_id, data, len);
}

void bt_start_scan(void) {
    printf("Starting inquiry scan..\r\n");
    gap_inquiry_start(INQUIRY_INTERVAL);
}

void bt_stop_scan(void) {
    printf("Stopping inquiry scan..\r\n");
    gap_inquiry_stop();
}

int bt_get_scan_results(uint8_t *buf) {
    int size = 0;
    for (int i = 0; i < deviceCount; i++) {
        int len = strnlen(devices[i].name_buffer, sizeof(devices[i].name_buffer));
        memcpy(buf + size, devices[i].name_buffer, len);
        size += len;
    }
    return size;
}

static int has_more_remote_name_requests(void) {
    int i;
    for (i = 0; i < deviceCount; i++) {
        if (devices[i].state == REMOTE_NAME_REQUEST) return 1;
    }
    return 0;
}

static void do_next_remote_name_request(void) {
    int i;
    for (i = 0; i < deviceCount; i++) {
        // remote name request
        if (devices[i].state == REMOTE_NAME_REQUEST) {
            devices[i].state = REMOTE_NAME_INQUIRED;
            printf("Get remote name of %s...\r\n", bd_addr_to_str(devices[i].address));
            gap_remote_name_request(devices[i].address, devices[i].pageScanRepetitionMode, devices[i].clockOffset | 0x8000);
            return;
        }
    }
}

static void continue_remote_names(void) {
    if (has_more_remote_name_requests()) {
        do_next_remote_name_request();
        return;
    }
}

static uint16_t vid = 0;
static uint16_t pid = 0;
static uint16_t version = 0;
static USB_Device_Type_t type;
static void hid_host_handle_interrupt_report(const uint8_t *report, uint16_t report_len) {
    tick_bluetooth_inputs(report + 1, report_len - 1, type);
}

static uint8_t attribute_value[1000];
static const int attribute_value_buffer_size = sizeof(attribute_value);
static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);
    uint8_t status;

    switch (hci_event_packet_get_type(packet)) {
        case SDP_EVENT_QUERY_ATTRIBUTE_VALUE:
            attribute_value[sdp_event_query_attribute_byte_get_data_offset(packet)] = sdp_event_query_attribute_byte_get_data(packet);
            if ((uint16_t)(sdp_event_query_attribute_byte_get_data_offset(packet) + 1) == sdp_event_query_attribute_byte_get_attribute_length(packet)) {
                if (sdp_event_query_attribute_byte_get_attribute_id(packet) == BLUETOOTH_ATTRIBUTE_VENDOR_ID) {
                    unsigned int pos = de_get_header_size(attribute_value);
                    vid = big_endian_read_16(attribute_value, pos);
                }
                if (sdp_event_query_attribute_byte_get_attribute_id(packet) == BLUETOOTH_ATTRIBUTE_PRODUCT_ID) {
                    unsigned int pos = de_get_header_size(attribute_value);
                    pid = big_endian_read_16(attribute_value, pos);
                }
                if (sdp_event_query_attribute_byte_get_attribute_id(packet) == BLUETOOTH_ATTRIBUTE_VERSION) {
                    unsigned int pos = de_get_header_size(attribute_value);
                    version = big_endian_read_16(attribute_value, pos);
                }
            }
            break;
        case SDP_EVENT_QUERY_COMPLETE:
            if (sdp_event_query_complete_get_status(packet)) {
                printf("SDP query failed 0x%02x\n", sdp_event_query_complete_get_status(packet));
                break;
            }
            type.sub_type = GAMEPAD;
            get_usb_device_type_for(vid, pid, version, &type);
            printf("Found device: %02x %02x\r\n", type.console_type, type.sub_type);
            printf("SDP query done.\n");

            status = hid_host_connect(remote_addr, hid_host_report_mode, &hid_host_cid);
            if (status != ERROR_CODE_SUCCESS) {
                printf("HID host connect failed, status 0x%02x.\r\n", status);
            }
            break;
        default:
            break;
    }
}
/*
 * @section Packet Handler
 *
 * @text The packet handler responds to various HID events.
 */

/* LISTING_START(packetHandler): Packet Handler */
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    /* LISTING_PAUSE */
    UNUSED(channel);
    UNUSED(size);

    uint8_t event;
    bd_addr_t event_addr;
    uint8_t status;

    bd_addr_t addr;
    int i;
    int index;

    /* LISTING_RESUME */
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            event = hci_event_packet_get_type(packet);

            switch (event) {
                /* @text When BTSTACK_EVENT_STATE with state HCI_STATE_WORKING
                 * is received and the example is started in client mode, the remote SDP HID query is started.
                 */
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                        if (has_address) {
                            // Grab vid and pid
                            sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr, BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
                        }
                        // bt_start_scan();
                    }
                    break;
                case GAP_EVENT_INQUIRY_RESULT: {
                    if (deviceCount >= MAX_DEVICES) break;  // already full
                    gap_event_inquiry_result_get_bd_addr(packet, addr);
                    index = getDeviceIndexForAddress(addr);
                    if (index >= 0) break;  // already in our list
                    uint32_t cod = gap_event_inquiry_result_get_class_of_device(packet);
                    if ((cod & PERIPHERAL_COD) != PERIPHERAL_COD) {
                        // Skip non peripheral devices
                        break;
                    }
                    memcpy(devices[deviceCount].address, addr, 6);
                    devices[deviceCount].pageScanRepetitionMode = gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
                    devices[deviceCount].clockOffset = gap_event_inquiry_result_get_clock_offset(packet);
                    devices[deviceCount].cod = cod;
                    if (gap_event_inquiry_result_get_name_available(packet)) {
                        int name_len = gap_event_inquiry_result_get_name_len(packet);
                        memcpy(devices[deviceCount].name_buffer, gap_event_inquiry_result_get_name(packet), name_len);
                        devices[deviceCount].name_buffer[name_len] = ' ';
                        devices[deviceCount].name_buffer[name_len + 1] = '(';
                        memcpy(devices[deviceCount].name_buffer + name_len + 2, bd_addr_to_str(addr), SIZE_OF_BD_ADDRESS);
                        devices[deviceCount].name_buffer[name_len + SIZE_OF_BD_ADDRESS + 1] = ')';
                        devices[deviceCount].name_buffer[name_len + SIZE_OF_BD_ADDRESS + 2] = 0;
                        printf("Found device '%s'\r\n", devices[deviceCount].name_buffer);
                        devices[deviceCount].state = REMOTE_NAME_FETCHED;
                    } else {
                        devices[deviceCount].state = REMOTE_NAME_REQUEST;
                    }
                    printf("\r\n");
                    deviceCount++;
                    break;
                }
                case GAP_EVENT_INQUIRY_COMPLETE:
                    for (i = 0; i < deviceCount; i++) {
                        // retry remote name request
                        if (devices[i].state == REMOTE_NAME_INQUIRED)
                            devices[i].state = REMOTE_NAME_REQUEST;
                    }
                    continue_remote_names();
                    break;

                case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
                    hci_event_remote_name_request_complete_get_bd_addr(packet, addr);
                    index = getDeviceIndexForAddress(addr);
                    if (index >= 0) {
                        if (packet[2] == 0) {
                            const char *name = hci_event_remote_name_request_complete_get_remote_name(packet);
                            int name_len = strlen(name);
                            memcpy(devices[deviceCount].name_buffer, name, name_len);
                            devices[deviceCount].name_buffer[name_len] = ' ';
                            devices[deviceCount].name_buffer[name_len + 1] = '(';
                            memcpy(devices[deviceCount].name_buffer + name_len + 2, bd_addr_to_str(addr), SIZE_OF_BD_ADDRESS);
                            devices[deviceCount].name_buffer[name_len + SIZE_OF_BD_ADDRESS + 1] = ')';
                            devices[deviceCount].name_buffer[name_len + SIZE_OF_BD_ADDRESS + 2] = 0;
                            printf("Found device '%s'\r\n", devices[deviceCount].name_buffer);
                            printf("Peripheral: %d\r\n", devices[deviceCount].cod & (1 << 8 | 1 << 10) == (1 << 8 | 1 << 10));
                            devices[index].state = REMOTE_NAME_FETCHED;
                        } else {
                            printf("Failed to get name: page timeout\r\n");
                        }
                    }
                    continue_remote_names();
                    break;

                default:
                    break;
                /* LISTING_PAUSE */
                case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    printf("Pin code request - using '0000'\r\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // inform about user confirmation request
                    printf("SSP User Confirmation Request with numeric value '%" PRIu32 "'\r\n", little_endian_read_32(packet, 8));
                    printf("SSP User Confirmation Auto accept\r\n");
                    break;

                /* LISTING_RESUME */
                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)) {
                        case HID_SUBEVENT_INCOMING_CONNECTION:
                            // There is an incoming connection: we can accept it or decline it.
                            // The hid_host_report_mode in the hid_host_accept_connection function
                            // allows the application to request a protocol mode.
                            // For available protocol modes, see hid_protocol_mode_t in btstack_hid.h file.
                            hid_host_accept_connection(hid_subevent_incoming_connection_get_hid_cid(packet), hid_host_report_mode);
                            break;

                        case HID_SUBEVENT_CONNECTION_OPENED:
                            // The status field of this event indicates if the control and interrupt
                            // connections were opened successfully.
                            status = hid_subevent_connection_opened_get_status(packet);
                            if (status != ERROR_CODE_SUCCESS) {
                                printf("Connection failed, status 0x%02x\r\n", status);
                                app_state = APP_IDLE;
                                hid_host_cid = 0;
                                return;
                            }
                            app_state = APP_CONNECTED;
                            hid_host_descriptor_available = false;
                            hid_host_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                            printf("HID Host connected.\r\n");
                            break;

                        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
                            // This event will follows HID_SUBEVENT_CONNECTION_OPENED event.
                            // For incoming connections, i.e. HID Device initiating the connection,
                            // the HID_SUBEVENT_DESCRIPTOR_AVAILABLE is delayed, and some HID
                            // reports may be received via HID_SUBEVENT_REPORT event. It is up to
                            // the application if these reports should be buffered or ignored until
                            // the HID descriptor is available.
                            status = hid_subevent_descriptor_available_get_status(packet);
                            if (status == ERROR_CODE_SUCCESS) {
                                hid_host_descriptor_available = true;
                                printf("HID Descriptor available, please start typing.\r\n");
                                // TODO: why is this so slow
                                // read full report
                                // hid_host_send_get_report(hid_host_cid, HID_REPORT_TYPE_FEATURE, 0x02);
                            } else {
                                printf("Cannot handle input report, HID Descriptor is not available, status 0x%02x\r\n", status);
                            }
                            break;

                        case HID_SUBEVENT_REPORT:
                            // Handle input report.
                            if (hid_host_descriptor_available) {
                                hid_host_handle_interrupt_report(hid_subevent_report_get_report(packet), hid_subevent_report_get_report_len(packet));
                            } else {
                                printf_hexdump(hid_subevent_report_get_report(packet), hid_subevent_report_get_report_len(packet));
                            }
                            break;
                        case HID_SUBEVENT_GET_REPORT_RESPONSE:
                            status = hid_subevent_get_protocol_response_get_handshake_status(packet);
                            if (status != HID_HANDSHAKE_PARAM_TYPE_SUCCESSFUL) {
                                printf("Error get report, status 0x%02x\r\n", status);
                                break;
                            }
                            break;

                        case HID_SUBEVENT_SET_PROTOCOL_RESPONSE:
                            // For incoming connections, the library will set the protocol mode of the
                            // HID Device as requested in the call to hid_host_accept_connection. The event
                            // reports the result. For connections initiated by calling hid_host_connect,
                            // this event will occur only if the established report mode is boot mode.
                            status = hid_subevent_set_protocol_response_get_handshake_status(packet);
                            if (status != HID_HANDSHAKE_PARAM_TYPE_SUCCESSFUL) {
                                printf("Error set protocol, status 0x%02x\r\n", status);
                                break;
                            }
                            switch ((hid_protocol_mode_t)hid_subevent_set_protocol_response_get_protocol_mode(packet)) {
                                case HID_PROTOCOL_MODE_BOOT:
                                    printf("Protocol mode set: BOOT.\r\n");
                                    break;
                                case HID_PROTOCOL_MODE_REPORT:
                                    printf("Protocol mode set: REPORT.\r\n");
                                    break;
                                default:
                                    printf("Unknown protocol mode.\r\n");
                                    break;
                            }
                            break;

                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            // The connection was closed.
                            hid_host_cid = 0;
                            hid_host_descriptor_available = false;
                            printf("HID Host disconnected.\r\n");
                            break;

                        default:
                            break;
                    }
                    break;
            }
            break;
        default:
            break;
    }
}
/* LISTING_END */

int btstack_main(void) {
    hid_host_setup();
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);

#ifdef BT_ADDR
#ifdef CONFIGURABLE_BLOBS
    has_address = bt_addr[0];
#else
    has_address = true;
#endif
    if (has_address) {
        // parse human readable Bluetooth address
        sscanf_bd_addr(remote_addr_string, remote_addr);
    }
#endif

    // Turn on the device
    hci_power_control(HCI_POWER_ON);
    return 0;
}

/* EXAMPLE_END */

#endif