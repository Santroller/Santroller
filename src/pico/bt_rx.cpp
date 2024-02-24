#include "config.h"
#ifdef BLUETOOTH_RX
#include <btstack_tlv.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Arduino.h"
#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "bt.h"
#include "bt_profile.h"
#include "btstack.h"
#include "btstack_config.h"
#include "config.h"
#include "endpoints.h"
#include "gap.h"
#include "hid.h"
#include "shared_main.h"

// TAG to store remote device address and type in TLV
#define TLV_TAG_HOGD ((((uint32_t)'H') << 24) | (((uint32_t)'O') << 16) | (((uint32_t)'G') << 8) | 'D')

typedef struct {
    bd_addr_t addr;
    bd_addr_type_t addr_type;
} le_device_addr_t;

static enum {
    W4_WORKING,
    W4_HID_DEVICE_FOUND,
    W4_CONNECTED,
    W4_ENCRYPTED,
    W4_HID_CLIENT_CONNECTED,
    READY,
    W4_TIMEOUT_THEN_SCAN,
    W4_TIMEOUT_THEN_RECONNECT,
} app_state;

static const uint8_t santroller_name[] = "SantrollerBT";
#ifdef CONFIGURABLE_BLOBS
static const char *bt_addr = (const char *)&BT_ADDR;
#else
#ifdef BT_ADDR
static const char bt_addr[] = BT_ADDR;
#endif
#endif
static bd_addr_t bt_addr_recv;
static le_device_addr_t remote_device;
static hci_con_handle_t connection_handle;
static bool has_address = false;
static uint16_t hids_cid;
static hid_protocol_mode_t protocol_mode = HID_PROTOCOL_MODE_REPORT;

// SDP
static uint8_t hid_descriptor_storage[500];
static uint8_t send_report_buf[64];
static uint8_t send_report_len = 0;

// used to implement connection timeout and reconnect timer
static btstack_timer_source_t connection_timer;
static btstack_timer_source_t reconnect_timer;
static btstack_timer_source_t scan_timer;

// register for events from HCI/GAP and SM
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// used to store remote device in TLV
static const btstack_tlv_t *btstack_tlv_singleton_impl;
static void *btstack_tlv_singleton_context;

static void hog_connect(void);
/**
 * @section Test if advertisement contains HID UUID
 * @param packet
 * @param size
 * @returns true if it does
 */
static bool adv_event_contains_hid_service(const uint8_t *packet) {
    const uint8_t *ad_data = gap_event_advertising_report_get_data(packet);
    uint8_t ad_len = gap_event_advertising_report_get_data_length(packet);
    return ad_data_contains_uuid16(ad_len, ad_data, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE);
}

static uint8_t scan_buffer[MAX_DEVICES_TO_SCAN * SIZE_OF_BD_ADDRESS];
static uint8_t devices_found;

int get_bt_address(uint8_t *addr) {
    bd_addr_t local_addr;
    gap_local_bd_addr(local_addr);
    memcpy(addr, bd_addr_to_str(local_addr), SIZE_OF_BD_ADDRESS);
    return SIZE_OF_BD_ADDRESS;
}

void bt_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id) {
    hids_client_send_write_report(hids_cid, 1, HID_REPORT_TYPE_OUTPUT, send_report_buf, send_report_len);
    memcpy(send_report_buf, data, len);
    send_report_len = len;
}

bool check_bluetooth_ready() {
#if DEVICE_TYPE == STAGE_KIT
    if (app_state == READY) {
        if (send_report_len) {
            hids_client_send_write_report(hids_cid, 1, HID_REPORT_TYPE_OUTPUT, send_report_buf, send_report_len);
        }
    }
#endif
    return app_state == READY;
}

void hog_stop_scan() {
    gap_stop_scan();
}

static void hog_stop_scan_timer(btstack_timer_source_t *ts) {
    UNUSED(ts);
    printf("stop scan\r\n");
    gap_stop_scan();
}
/**
 * Start scanning
 */
void hog_start_scan() {
    printf("Scanning for LE HID devices...\r\n");
    devices_found = 0;
#ifdef BT_ADDR
    if (has_address) {
        memcpy(scan_buffer, bt_addr, SIZE_OF_BD_ADDRESS);
        devices_found = 1;
    }
#endif
    // Passive scanning, 100% (scan interval = scan window)
    gap_set_scan_parameters(0, 48, 48);
    gap_start_scan();
    // Auto stop scanning after 10 seconds
    btstack_run_loop_set_timer(&scan_timer, 10000);
    btstack_run_loop_set_timer_handler(&scan_timer, &hog_stop_scan_timer);
    btstack_run_loop_add_timer(&scan_timer);
}

int hog_get_scan_results(uint8_t *buf) {
    memcpy(buf, scan_buffer, devices_found * SIZE_OF_BD_ADDRESS);
    return devices_found * SIZE_OF_BD_ADDRESS;
}

/**
 * Handle timer event to trigger reconnect
 * @param ts
 */
static void hog_reconnect_timeout(btstack_timer_source_t *ts) {
    UNUSED(ts);
    hog_connect();
}
/**
 * Handle timeout for outgoing connection
 * @param ts
 */
static void hog_connection_timeout(btstack_timer_source_t *ts) {
    UNUSED(ts);
    printf("Timeout - abort connection\r\n");
    gap_connect_cancel();

    btstack_run_loop_set_timer(&reconnect_timer, 100);
    btstack_run_loop_set_timer_handler(&reconnect_timer, &hog_reconnect_timeout);
    btstack_run_loop_add_timer(&reconnect_timer);
}

/**
 * Connect to remote device but set timer for timeout
 */
static void hog_connect(void) {
    // set timer
    btstack_run_loop_set_timer(&connection_timer, 10000);
    btstack_run_loop_set_timer_handler(&connection_timer, &hog_connection_timeout);
    btstack_run_loop_add_timer(&connection_timer);
    app_state = W4_CONNECTED;
    gap_connect(remote_device.addr, remote_device.addr_type);
}

static void hog_start_reconnect_timer() {
    btstack_run_loop_set_timer(&connection_timer, 100);
    btstack_run_loop_set_timer_handler(&connection_timer, &hog_reconnect_timeout);
    btstack_run_loop_add_timer(&connection_timer);
}

/**
 * Start connecting after boot up: connect to last used device if possible, start scan otherwise
 */
static void hog_start_connect(void) {
    if (has_address) {
    }
    // Only try to connect if we have "paired" a device
#ifdef BT_ADDR
#ifdef CONFIGURABLE_BLOBS
    // If the address starts with a null byte, then it is not actually set and should be ignored.
    if (scan_buffer[0]) {
        return;
    }
#endif
    hog_connect();
#endif
}

/**
 * In case of error, disconnect and start scanning again
 */
static void handle_outgoing_connection_error(void) {
    printf("Error occurred, disconnect and start over\r\n");
    gap_disconnect(connection_handle);
    hog_start_reconnect_timer();
}
/**
 * Handle GATT Client Events dependent on current state
 *
 * @param packet_type
 * @param channel
 * @param packet
 * @param size
 */
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t status;

    if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) {
        return;
    }

    switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
        case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED:
            status = gattservice_subevent_hid_service_connected_get_status(packet);
            switch (status) {
                case ERROR_CODE_SUCCESS:
                    printf("HID service client connected, found %d services\r\n",
                           gattservice_subevent_hid_service_connected_get_num_instances(packet));

                    // store device as bonded
                    if (btstack_tlv_singleton_impl) {
                        btstack_tlv_singleton_impl->store_tag(btstack_tlv_singleton_context, TLV_TAG_HOGD, (const uint8_t *)&remote_device, sizeof(remote_device));
                    }

                    hids_client_get_hid_information(hids_cid, 0);
                    // done
                    printf("Ready - receiving data\r\n");
                    app_state = READY;
                    break;
                default:
                    printf("HID service client connection failed, err 0x%02x.\r\n", status);
                    handle_outgoing_connection_error();
                    break;
            }
            break;
        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_DONE:
            printf("info done\r\n");
            gap_update_connection_parameters(connection_handle, 6, 6, 0, 100);
            hids_client_connect(connection_handle, handle_gatt_client_event, protocol_mode, &hids_cid);
            break;
        case GATTSERVICE_SUBEVENT_HID_REPORT: {
            tick_bluetooth(gattservice_subevent_hid_report_get_report(packet), gattservice_subevent_hid_report_get_report_len(packet));
            break;
        }

        default:
            break;
    }
}

/* LISTING_START(packetHandler): Packet Handler */
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    /* LISTING_PAUSE */
    UNUSED(channel);
    UNUSED(size);
    uint8_t event;
    /* LISTING_RESUME */
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            event = hci_event_packet_get_type(packet);
            switch (event) {
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) break;
                    btstack_assert(app_state == W4_WORKING);

                    hog_start_connect();
                    break;
                case GAP_EVENT_ADVERTISING_REPORT: {
                    if (adv_event_contains_hid_service(packet) == false) break;
                    // store remote device address and type
                    bd_addr_t address;
                    gap_event_advertising_report_get_address(packet, address);
                    const uint8_t *adv_data = gap_event_advertising_report_get_data(packet);
                    uint8_t adv_size = gap_event_advertising_report_get_data_length(packet);
                    ad_context_t context;
                    // Scan for devices, and if they are santrollers then store their mac addresses.
                    for (ad_iterator_init(&context, adv_size, adv_data); ad_iterator_has_more(&context); ad_iterator_next(&context)) {
                        uint8_t data_type = ad_iterator_get_data_type(&context);
                        uint8_t size = ad_iterator_get_data_len(&context);
                        const uint8_t *data = ad_iterator_get_data(&context);
                        if (data_type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME && memcmp(data, santroller_name, sizeof(santroller_name) - 1) == 0) {
                            char *address_string = bd_addr_to_str(address);
                            bool found = false;
                            for (int i = 0; i < devices_found; i++) {
                                if (memcmp(address_string, scan_buffer + (i * SIZE_OF_BD_ADDRESS), SIZE_OF_BD_ADDRESS) == 0) {
                                    found = true;
                                }
                            }
                            if (!found) {
                                printf("Found, device address %s ...\r\n", address_string);
                                memcpy(scan_buffer + (devices_found * SIZE_OF_BD_ADDRESS), address_string, SIZE_OF_BD_ADDRESS);
                                devices_found++;
                            }
                        }
                    }
                    break;
                }
                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    if (app_state != READY) break;
                    hids_client_disconnect(hids_cid);
                    connection_handle = HCI_CON_HANDLE_INVALID;
                    switch (app_state) {
                        case READY:
                            printf("\r\nDisconnected, try to reconnect...\r\n");
                            app_state = W4_TIMEOUT_THEN_RECONNECT;
                            break;
                        default:
                            printf("\r\nDisconnected, start over...\r\n");
                            app_state = W4_TIMEOUT_THEN_SCAN;
                            break;
                    }
                    hog_start_reconnect_timer();
                    break;

                case HCI_EVENT_LE_META:
                    switch (hci_event_le_meta_get_subevent_code(packet)) {
                        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                            // print connection parameters (without using float operations)
                            uint16_t conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
                            printf("LE Connection Complete:\r\n");
                            printf("- Connection Interval: %u.%02u ms\r\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                            printf("- Connection Latency: %u\r\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
                            break;
                        }
                        case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE: {
                            // print connection parameters (without using float operations)
                            uint16_t conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
                            printf("LE Connection Update:\r\n");
                            printf("- Connection Interval: %u.%02u ms\r\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                            printf("- Connection Latency: %u\r\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                            break;
                        }
                        default:
                            break;
                    }
                    // wait for connection complete
                    if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) break;
                    if (app_state != W4_CONNECTED) return;
                    btstack_run_loop_remove_timer(&connection_timer);
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    // request security
                    app_state = W4_ENCRYPTED;
                    sm_request_pairing(connection_handle);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}
/* LISTING_END */

/* @section HCI packet handler
 *
 * @text The SM packet handler receives Security Manager Events required for pairing.
 * It also receives events generated during Identity Resolving
 * see Listing SMPacketHandler.
 */

/* LISTING_START(SMPacketHandler): Scanning and receiving advertisements */

static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("Just works requested\r\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            printf("Confirming numeric comparison: %" PRIu32 "\r\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            printf("Display Passkey: %" PRIu32 "\r\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            switch (sm_event_pairing_complete_get_status(packet)) {
                case ERROR_CODE_SUCCESS:
                    printf("Pairing complete, success\r\n");

                    // continue - query primary services
                    printf("Search for HID service.\r\n");
                    app_state = W4_HID_CLIENT_CONNECTED;
                    device_information_service_client_query(connection_handle, handle_gatt_client_event);
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    printf("Pairing failed, timeout\r\n");
                    hog_start_reconnect_timer();
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    printf("Pairing failed, disconnected\r\n");
                    hog_start_reconnect_timer();
                    break;
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                    printf("Pairing failed, reason = %u\r\n", sm_event_pairing_complete_get_reason(packet));
                    hog_start_reconnect_timer();
                    break;
                default:
                    break;
            }
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE:
            printf("Re-encryption complete, success\n");
            app_state = W4_HID_CLIENT_CONNECTED;
            device_information_service_client_query(connection_handle, handle_gatt_client_event);
            break;
        default:
            break;
    }
}
/* LISTING_END */

int btstack_main(void) {
    printf("Bt init\r\n");
#ifdef BT_ADDR
#ifdef CONFIGURABLE_BLOBS
    has_address = bt_addr[0];
#else
    has_address = true;
#endif
    if (has_address) {
        sscanf_bd_addr(bt_addr, remote_device.addr);
        remote_device.addr_type = BD_ADDR_TYPE_LE_PUBLIC;
    }
#endif
    /* LISTING_START(HogBootHostSetup): HID-over-GATT Host Setup */

    // register for events from HCI
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for events from Security Manager
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    //
    l2cap_init();
    sm_init();
    gatt_client_init();

    hids_client_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));

    app_state = W4_WORKING;

    // Turn on the device
    hci_power_control(HCI_POWER_ON);
    return 0;
}

/* EXAMPLE_END */
#endif