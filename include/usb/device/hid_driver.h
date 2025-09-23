#include "stdint.h"
#include "tusb.h"
#include "usb/usb_descriptors.h"
typedef struct {
  char const* name;
  ConsoleMode mode;
  uint16_t (* open             ) (uint8_t rhport, tusb_desc_interface_t const * desc_intf, uint16_t max_len);
  bool     (* control_xfer_cb  ) (uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);
  uint16_t     (* get_report_cb    ) (uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
  void     (* set_report_cb    ) (uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
} hidd_driver_t;

typedef struct {
  char const* name;
  ConsoleMode mode;
  uint16_t (* open             ) (uint8_t rhport, tusb_desc_interface_t const * desc_intf, uint16_t max_len);
  bool     (* control_xfer_cb  ) (uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);
  uint16_t     (* get_report_cb    ) (uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
  void     (* set_report_cb    ) (uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
} hidh_driver_t;
