#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "host/usbh.h"
#include "host/usbh_classdriver.h"
#include "xinput_host.h"
#include "tusb.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
void print_greeting(void);
void led_blinking_task(void);

extern void cdc_task(void);
extern void hid_app_task(void);
#define UART_ID uart1
#define BAUD_RATE 115200

// We are using pins 4 and 5, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 4
#define UART_RX_PIN 5
uint8_t dev_addr;
uint8_t buffer[256];
bool done2(uint8_t dev_addr, tusb_control_request_t const *request,
           xfer_result_t result) {
  printf("Data written, cb recieved");
  return true;
}
bool done(uint8_t dev_addr, tusb_control_request_t const *request,
          xfer_result_t result) {
  printf("Data read, writing back to xbox");
  uart_putc_raw(UART_ID, 0x7F);
  uint16_t len = request->wLength;
  uint8_t *b = buffer;
  printf("data: ");
  while (len--) {
    printf("%x ", *b);
    uart_putc_raw(UART_ID, *(b++));
  }
  return true;
}
/*------------- MAIN -------------*/
int main(void) {
  board_init();

  tusb_init();
  uart_init(UART_ID, BAUD_RATE);
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  while (1) {
    // tinyusb host task
    tuh_task();
    if (uart_is_readable(UART_ID)) {
      tusb_control_request_t request;
      uint8_t buf[sizeof(tusb_control_request_t)];
      uart_read_blocking(UART_ID, buf, 1);
      if (buf[0] == 0xC1 || buf[0] == 0x41) {
        printf("Received Vendor!\n");
        uart_read_blocking(UART_ID, buf + 1,
                           sizeof(tusb_control_request_t) - 1);
        memcpy(&request, buf, sizeof(tusb_control_request_t));
        printf("Received: ");
        for (int i = 0; i < sizeof(tusb_control_request_t); i++) {
          printf("%x ", buf[i]);
        }
        printf("\n");
        printf("type: %x, bRequest: %x, dir: %x, index: %x, val: %x\n",
               request.bmRequestType_bit.type, request.bRequest,
               request.bmRequestType_bit.direction, request.wIndex,
               request.wValue);
        if (request.bmRequestType_bit.direction ==
            TUSB_DIR_OUT) { // Host to device
          printf("waiting for data\b");
          uart_read_blocking(UART_ID, (uint8_t *)&buffer, request.wLength);
          printf("got data\b");
          tuh_control_xfer(dev_addr, &request, buffer, &done2);
        } else { // Device to host
          tuh_control_xfer(dev_addr, &request, buffer, &done);
        }
      }
    }
  }

  return 0;
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *report_desc, uint16_t desc_len) {}
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const *report, uint16_t len) {}
bool tusbh_xinput_mount_cb(uint8_t rhport, uint8_t d,
                           tusb_desc_interface_t const *itf_desc,
                           uint16_t *p_length) {
  dev_addr = d;
  return true;
}
usbh_class_driver_t driver[] = {{.init = xinputh_init,
                                 .open = xinputh_open_subtask,
                                 .set_config = xinputh_set_config,
                                 .xfer_cb = xinputh_isr,
                                 .close = xinputh_close}};
usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;
  return driver;
}