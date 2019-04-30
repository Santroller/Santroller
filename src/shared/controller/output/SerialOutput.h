#pragma once
/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "../../../config/eeprom.h"
extern "C" {
#include "../lufa/Descriptors.h"
}
#include "Output.h"
#include "stdint.h"

/* Macros: */
/** Endpoint address of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPADDR (ENDPOINT_DIR_IN | 2)

/** Endpoint address of the CDC device-to-host data IN endpoint. */
#define CDC_TX_EPADDR (ENDPOINT_DIR_IN | 3)

/** Endpoint address of the CDC host-to-device data OUT endpoint. */
#define CDC_RX_EPADDR (ENDPOINT_DIR_OUT | 4)

/** Size in bytes of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPSIZE 8

/** Size in bytes of the CDC data IN and OUT endpoints. */
#define CDC_TXRX_EPSIZE 16

/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum CDC_InterfaceDescriptors_t {
  INTERFACE_ID_CDC_CCI = 0, /**< CDC CCI interface descriptor ID */
  INTERFACE_ID_CDC_DCI = 1, /**< CDC DCI interface descriptor ID */
};

typedef struct {
  USB_Descriptor_Configuration_Header_t Config;

  // CDC Control Interface
  USB_Descriptor_Interface_t CDC_CCI_Interface;
  USB_CDC_Descriptor_FunctionalHeader_t CDC_Functional_Header;
  USB_CDC_Descriptor_FunctionalACM_t CDC_Functional_ACM;
  USB_CDC_Descriptor_FunctionalUnion_t CDC_Functional_Union;
  USB_Descriptor_Endpoint_t CDC_NotificationEndpoint;

  // CDC Data Interface
  USB_Descriptor_Interface_t CDC_DCI_Interface;
  USB_Descriptor_Endpoint_t CDC_DataOutEndpoint;
  USB_Descriptor_Endpoint_t CDC_DataInEndpoint;
} USB_CDC_Descriptor_Configuration_t;

class SerialOutput : public Output {
public:
  void init();
  void update(Controller controller);
  void usb_connect();
  void usb_disconnect();
  void usb_configuration_changed();
  void usb_control_request();
  void usb_start_of_frame();
  uint16_t get_descriptor(const uint8_t DescriptorType,
                          const uint8_t DescriptorNumber,
                          const void **const DescriptorAddress,
                          uint8_t *const DescriptorMemorySpace);
};