#include "response.h"
#define COUNTER_INCREMENT 3

// Private functions (definition)
static void prepare_reply(uint8_t code, uint8_t command, uint8_t data[], uint8_t length);
static void prepare_uart_reply(uint8_t code, uint8_t subcommand, uint8_t data[], uint8_t length);
static void prepare_spi_reply(SPI_Address_t address, size_t size);
static void prepare_standard_report(USB_SwitchStandardReport_t *standardReport);
static void prepare_8101(void);

// Variables
uint8_t mac_address[] = {0x79, 0x05, 0x44, 0xC6, 0xB5, 0x65};
static uint8_t replyBuffer[DEVICE_EPSIZE_IN];
static uint8_t counter = 0;
static bool nextPacketReady = false;
static USB_SwitchStandardReport_t selectedReportPtr;

void setup_response_manager(void) {

    // Initial value for IN endpoint buffer
    prepare_8101();
}

void process_OUT_report(uint8_t* ReportData, uint8_t ReportSize) {
    // https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md
    if (ReportData[0] == 0x80) {
        switch (ReportData[1]) {
            case 0x01: {
                prepare_8101();
                break;
            }
            case 0x02:
            case 0x03: {
                prepare_reply(0x81, ReportData[1], NULL, 0);
                break;
            }
            case 0x04: {
                //reportType = NONE;
                prepare_standard_report(&selectedReportPtr);
                break;
            }
            default: {
                // TODO
                prepare_reply(0x81, ReportData[1], NULL, 0);
                break;
            }
        }
    } else if (ReportData[0] == 0x01 && ReportSize > 16) {
        Switch_Subcommand_t subcommand = (Switch_Subcommand_t)ReportData[10];
        switch (subcommand) {
            case SUBCOMMAND_BLUETOOTH_MANUAL_PAIRING: {
                prepare_uart_reply(0x81, subcommand, (uint8_t *) 0x03, 1);
                break;
            }
            case SUBCOMMAND_REQUEST_DEVICE_INFO: {
                size_t n = sizeof(mac_address); // = 6
                uint8_t buf[n + 6];
                buf[0] = 0x03; buf[1] = 0x48; // Firmware version
                buf[2] = 0x03; // Pro Controller
                buf[3] = 0x02; // Unkown
                // MAC address is flipped (big-endian)
                for (unsigned int i = 0; i < n; i++) {
                    buf[(n + 3) - i] = mac_address[i];
                }
                buf[n + 4] = 0x03; // Unknown
                buf[n + 5] = 0x02; // Use colors in SPI memory, and use grip colors (added in Switch firmware 5.0)
                prepare_uart_reply(0x82, subcommand, buf, sizeof(buf));
                break;
            }
            case SUBCOMMAND_SET_INPUT_REPORT_MODE:
            case SUBCOMMAND_SET_SHIPMENT_LOW_POWER_STATE:
            case SUBCOMMAND_SET_PLAYER_LIGHTS:
            case SUBCOMMAND_SET_HOME_LIGHTS:
            case SUBCOMMAND_ENABLE_IMU:
            case SUBCOMMAND_ENABLE_VIBRATION: {
                prepare_uart_reply(0x80, subcommand, NULL, 0);
                break;
            }
            case SUBCOMMAND_TRIGGER_BUTTONS_ELAPSED_TIME: {
                prepare_uart_reply(0x83, subcommand, NULL, 0);
                break;
            }
            case SUBCOMMAND_SET_NFC_IR_MCU_CONFIG: {
                uint8_t buf[] = {0x01, 0x00, 0xFF, 0x00, 0x03, 0x00, 0x05, 0x01};
                prepare_uart_reply(0xA0, subcommand, buf, sizeof(buf));
                break;
            }
            case SUBCOMMAND_SPI_FLASH_READ: {
                // SPI
                // Addresses are little-endian, so 80 60 means address 0x6080
                SPI_Address_t address = (ReportData[12] << 8) | ReportData[11];
                size_t size = (size_t) ReportData[15];
                prepare_spi_reply(address, size);
                break;
            }
            default: {
                // TODO
                prepare_uart_reply(0x80, subcommand, NULL, 0);
                break;
            }
        }
    }
}

void send_IN_report(void) {

    if (!nextPacketReady) {
        // No requests from Switch, use standard report
        prepare_standard_report(&selectedReportPtr);
    }

    // Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
    // while (!Endpoint_IsINReady()); // Wait until IN endpoint is ready
    // while (Endpoint_Write_Stream_LE(&replyBuffer, sizeof(replyBuffer), NULL) != ENDPOINT_RWSTREAM_NoError);
    // Endpoint_ClearIN(); // We then send an IN packet on this endpoint.

    nextPacketReady = false;
}

/*
 * Private functions (implementation)
 */

static void prepare_reply(uint8_t code, uint8_t command, uint8_t data[], uint8_t length) {
    if (nextPacketReady) return;
    memset(replyBuffer, 0, sizeof(replyBuffer));
    replyBuffer[0] = code;
    replyBuffer[1] = command;
    memcpy(&replyBuffer[2], &data[0], length);
    nextPacketReady = true;
}

static void prepare_uart_reply(uint8_t code, uint8_t subcommand, uint8_t data[], uint8_t length) {
    if (nextPacketReady) return;
    memset(replyBuffer, 0, sizeof(replyBuffer));
    replyBuffer[0] = 0x21;

    counter += COUNTER_INCREMENT;
    replyBuffer[1] = counter;

    USB_SwitchStandardReport_t *selectedReport = &selectedReportPtr;
    size_t n = sizeof(USB_SwitchStandardReport_t);
    memcpy(&replyBuffer[2], selectedReport, n);
    replyBuffer[n + 2] = code;
    replyBuffer[n + 3] = subcommand;
    memcpy(&replyBuffer[n + 4], &data[0], length);
    nextPacketReady = true;
}

static void prepare_spi_reply(SPI_Address_t address, size_t size) {
    uint8_t data[size];
    // Populate buffer with data read from SPI flash
    spi_read(address, size, data);

    uint8_t spiReplyBuffer[5 + size];
    // Big-endian
    spiReplyBuffer[0] = address & 0xFF;
    spiReplyBuffer[1] = (address >> 8) & 0xFF;
    spiReplyBuffer[2] = 0x00;
    spiReplyBuffer[3] = 0x00;
    spiReplyBuffer[4] = size;
    memcpy(&spiReplyBuffer[5], &data[0], size);

    prepare_uart_reply(0x90, SUBCOMMAND_SPI_FLASH_READ, spiReplyBuffer, sizeof(spiReplyBuffer));
}

static void prepare_standard_report(USB_SwitchStandardReport_t *standardReport) {
    if (nextPacketReady) return;
    counter += COUNTER_INCREMENT;
    prepare_reply(0x30, counter, (uint8_t *) standardReport, sizeof(USB_SwitchStandardReport_t));
}

static void prepare_8101(void) {
    if (nextPacketReady) return;
    size_t n = sizeof(mac_address); // = 6
    uint8_t buf[n + 2];
    buf[0] = 0x00;
    buf[1] = 0x03; // Pro Controller
    memcpy(&buf[2], &mac_address[0], n);
    prepare_reply(0x81, 0x01, buf, sizeof(buf));
}