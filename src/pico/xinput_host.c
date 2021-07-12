/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#  include "host/usbh.h"
#  include "host/usbh_classdriver.h"
#  include "output/descriptors.h"
#  include "xinput_host.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

typedef struct {
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;

  uint16_t report_size;
} xinputh_interface_t;

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
xinputh_interface_t xinput_interface[CFG_TUH_XINPUT];

static tusb_error_t xinputh_validate_paras(uint8_t dev_addr, uint16_t vendor_id,
                                           uint16_t product_id, void *p_buffer,
                                           uint16_t length) {
  if (!tusbh_xinput_is_mounted(dev_addr, vendor_id, product_id)) {
    return TUSB_ERROR_DEVICE_NOT_READY;
  }

  TU_ASSERT(p_buffer != NULL && length != 0, TUSB_ERROR_INVALID_PARA);

  return TUSB_ERROR_NONE;
}
//--------------------------------------------------------------------+
// APPLICATION API (need to check parameters)
//--------------------------------------------------------------------+
tusb_error_t tusbh_xinput_read(uint8_t dev_addr, uint16_t vendor_id,
                               uint16_t product_id, void *p_buffer,
                               uint16_t length) {
  // TU_ASSERT_ERR( xinputh_validate_paras(dev_addr, vendor_id, product_id,
  // p_buffer, length) );

  // if ( !hcd_pipe_is_idle(xinput_interface[dev_addr-1].pipe_in) )
  // {
  //   return TUSB_ERROR_INTERFACE_IS_BUSY;
  // }

  //   (void)usbh_edpt_xfer(dev_addr, xinput_interface[dev_addr - 1].ep_in,
  //   p_buffer,
  //                        length);

  return TUSB_ERROR_NONE;
}

tusb_error_t tusbh_xinput_write(uint8_t dev_addr, uint16_t vendor_id,
                                uint16_t product_id, void const *p_data,
                                uint16_t length) {
  // TU_ASSERT_ERR( xinputh_validate_paras(dev_addr, vendor_id, product_id,
  // p_data, length) );

  // if ( !hcd_pipe_is_idle(xinput_interface[dev_addr-1].pipe_out) )
  // {
  //   return TUSB_ERROR_INTERFACE_IS_BUSY;
  // }

  //   (void)usbh_edpt_xfer(dev_addr, xinput_interface[dev_addr - 1].ep_out,
  //   p_data,
  //                        length);

  return TUSB_ERROR_NONE;
}

//--------------------------------------------------------------------+
// USBH-CLASS API
//--------------------------------------------------------------------+
void xinputh_init(void) {
  tu_memclr(&xinput_interface, sizeof(xinputh_interface_t) * CFG_TUH_XINPUT);
}

static bool set_led_complete(uint8_t dev_addr,
                             tusb_control_request_t const *request,
                             xfer_result_t result) {
  printf("LED DONE!");
}

bool xinputh_open_subtask(uint8_t rhport, uint8_t dev_addr,
                          tusb_desc_interface_t const *itf_desc,
                          uint16_t *p_length) {
  TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
  uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                     (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

  // Find available interface
  xinputh_interface_t *p_xinput = NULL;
  for (uint8_t i = 0; i < CFG_TUH_XINPUT; i++) {
    if (xinput_interface[i].ep_in == 0 && xinput_interface[i].ep_out == 0) {
      p_xinput = &xinput_interface[i];
      break;
    }
  }
  TU_VERIFY(p_xinput, 0);

  uint8_t const *p_desc = (uint8_t const *)itf_desc;
  if (itf_desc->bInterfaceSubClass == 0x5D &&
      (itf_desc->bInterfaceProtocol == 0x01 ||
       itf_desc->bInterfaceProtocol == 0x03 ||
       itf_desc->bInterfaceProtocol == 0x02)) {
    // Xinput reserved endpoint
    //-------------- Xinput Descriptor --------------//
    p_desc = tu_desc_next(p_desc);
    USB_HID_XBOX_Descriptor_HID_t *x_desc =
        (USB_HID_XBOX_Descriptor_HID_t *)p_desc;
    TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->Header.Type, 0);
    drv_len += x_desc->Header.Size;
    uint8_t endpoints = itf_desc->bNumEndpoints;
    while (endpoints--) {

      p_desc = tu_desc_next(p_desc);
      tusb_desc_endpoint_t const *desc_ep =
          (tusb_desc_endpoint_t const *)p_desc;
      TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
      if (desc_ep->bEndpointAddress & 0x80) {
        p_xinput->ep_in = desc_ep->bEndpointAddress;
        TU_ASSERT(usbh_edpt_open(rhport, dev_addr, desc_ep));
      } else {
        p_xinput->ep_out = desc_ep->bEndpointAddress;
        TU_ASSERT(usbh_edpt_open(rhport, dev_addr, desc_ep));
      }
    }
    p_xinput->itf_num = itf_desc->bInterfaceNumber;
    // Prepare for incoming data
    // if (p_xinput->ep_out != 0xFF) {
    //   if (!usbh_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
    //                       sizeof(p_xinput->epout_buf))) {
    //     TU_LOG_FAILED();
    //     TU_BREAKPOINT();
    //   }
    // }

  } else if (itf_desc->bInterfaceSubClass == 0xfD &&
             itf_desc->bInterfaceProtocol == 0x13) {
    // Xinput reserved endpoint
    //-------------- Xinput Descriptor --------------//
    p_desc = tu_desc_next(p_desc);
    USB_HID_XBOX_Descriptor_HID_t *x_desc =
        (USB_HID_XBOX_Descriptor_HID_t *)p_desc;
    TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->Header.Type, 0);
    drv_len += x_desc->Header.Size;
    p_desc = tu_desc_next(p_desc);
  }
  *p_length = drv_len;
  return tusbh_xinput_mount_cb(rhport, dev_addr, itf_desc, p_length);
}
uint8_t init_led[] = {0x01, 0x03, 0x02};
bool xinputh_set_config(uint8_t dev_addr, uint8_t itf_num) {
  printf("Sending LEDS!\n");

  //   uint8_t buf[] = {0x01, 0x03, 0x02};
  //   (void)usbh_edpt_xfer(dev_addr, 0x02, buf, 3);
  uint8_t report_type = 0x02;
  uint8_t report_id = 0x00;
  tusb_control_request_t const request = {
      .bmRequestType_bit = {.recipient = TUSB_REQ_RCPT_INTERFACE,
                            .type = TUSB_REQ_TYPE_CLASS,
                            .direction = TUSB_DIR_OUT},
      .bRequest = HID_REQ_SetReport,
      .wValue = tu_u16(report_type, report_id),
      .wIndex = 0x00,
      .wLength = sizeof(init_led)};
  tuh_control_xfer(dev_addr, &request, init_led, set_led_complete);

  return true;
}
bool xinputh_isr(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result,
                 uint32_t xferred_bytes) {
  printf("Got Data: %d %d %d %d", dev_addr, ep_addr, result, xferred_bytes);
  return true;
}

void xinputh_close(uint8_t dev_addr) {
  tusb_error_t err1, err2;
  xinputh_interface_t *p_interface = &xinput_interface[dev_addr - 1];

  // // TODO re-consider to check pipe valid before calling pipe_close
  // if( pipehandle_is_valid( p_interface->pipe_in ) )
  // {
  //   err1 = hcd_pipe_close( p_interface->pipe_in );
  // }

  // if ( pipehandle_is_valid( p_interface->pipe_out ) )
  // {
  //   err2 = hcd_pipe_close( p_interface->pipe_out );
  // }

  tu_memclr(p_interface, sizeof(xinputh_interface_t));

  TU_ASSERT(err1 == TUSB_ERROR_NONE && err2 == TUSB_ERROR_NONE, (void)0);
}

#endif
