#include "XInputOutput.h"
#include "../../bootloader/Bootloader.h"
void XInputOutput::usb_connect() {}
void XInputOutput::usb_disconnect() {}

USB_JoystickReport_Data_t gamepad_state;

void XInputOutput::init() {
  ConfigurationDescriptor = {
    Config : {
      Header : {
        Size : sizeof(USB_Descriptor_Configuration_Header_t),
        Type : DTYPE_Configuration
      },

      TotalConfigurationSize : sizeof(Xinput_Descriptor_Configuration_t),
      TotalInterfaces : 1,

      ConfigurationNumber : 1,
      ConfigurationStrIndex : NO_DESCRIPTOR,

      ConfigAttributes : USB_CONFIG_ATTR_REMOTEWAKEUP,

      MaxPowerConsumption : USB_CONFIG_POWER_MA(500)
    },

    Interface0 : {
      Header :
          {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

      InterfaceNumber : 0,
      AlternateSetting : 0x00,

      TotalEndpoints : 2,

      Class : 0xFF,
      SubClass : 0x5D,
      Protocol : 0x01,

      InterfaceStrIndex : NO_DESCRIPTOR
    },

    XInputUnknown : {
      Header : {Size : sizeof(USB_HID_XBOX_Descriptor_HID_t), Type : 0x21},
      {0x00, 0x00, config.subtype, 0x25, 0x81, 0x14, 0x03, 0x03, 0x03, 0x04,
       0x13, 0x02, 0x08, 0x03, 0x00}
    },

    DataInEndpoint0 : {
      Header :
          {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

      EndpointAddress : 0x81,
      Attributes : EP_TYPE_INTERRUPT,
      EndpointSize : XBOX_EPSIZE,
      PollingIntervalMS : config.pollrate
    },
    DataOutEndpoint0 : {
      Header :
          {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

      EndpointAddress : 0x02,
      Attributes : EP_TYPE_INTERRUPT,
      EndpointSize : XBOX_EPSIZE,
      PollingIntervalMS : config.pollrate
    },
  };
  memset(&gamepad_state, 0x00, sizeof(USB_JoystickReport_Data_t));
  gamepad_state.rsize = 20;
  USB_Init();
  sei();
}
void XInputOutput::usb_configuration_changed() {
  Endpoint_ConfigureEndpoint(JOYSTICK_EPADDR_IN, EP_TYPE_INTERRUPT, 20, 1);
  Endpoint_ConfigureEndpoint((ENDPOINT_DIR_IN | 3), EP_TYPE_INTERRUPT, 32, 1);
}
void sendControl(uint8_t *out, uint8_t outSize) {
  Endpoint_ClearStall();
  Endpoint_ClearSETUP();
  /* Write the report data to the control endpoint */
  Endpoint_Write_Control_Stream_LE(out, outSize);
  Endpoint_ClearOUT();
}
void XInputOutput::usb_control_request() {
  const void *DescriptorPointer;
  uint16_t DescriptorSize;
  /* Handle HID Class specific requests */
  switch (USB_ControlRequest.bRequest) {
  case HID_REQ_GetReport:
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();

      /* Write the report data to the control endpoint */
      Endpoint_Write_Control_Stream_LE(&gamepad_state, 20);
      Endpoint_ClearOUT();
    }

    if (USB_ControlRequest.wLength == 0x04) {
      uint8_t data[4]; // DeviceID
      sendControl(data, sizeof(data));
    }
    if (USB_ControlRequest.wLength == 8 &&
        USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
      uint8_t data[8] = {0x00, 0x08}; // Flags
      sendControl(data, sizeof(data));
    }
    if (USB_ControlRequest.wLength == 20 &&
        USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
      uint8_t data[20]; // Capabilities
      sendControl(data, sizeof(data));
    }

    break;
  case REQ_GetOSFeatureDescriptor:
    if ((USB_ControlRequest.bmRequestType &
         (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
        (REQDIR_DEVICETOHOST | REQTYPE_VENDOR)) {

      DescriptorSize = USB_GetOSFeatureDescriptor(
          USB_ControlRequest.wValue >> 8, USB_ControlRequest.wIndex,
          USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT,
          &DescriptorPointer);
      if (DescriptorSize == NO_DESCRIPTOR)
        return;
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(DescriptorPointer, DescriptorSize);
      Endpoint_ClearOUT();
    }
  }
}
void XInputOutput::usb_start_of_frame() {}
void XInputOutput::update(Controller controller) {
  USB_USBTask();
  wdt_reset();
  /* Device must be connected and configured for the task to run */
  if (USB_DeviceState != DEVICE_STATE_Configured)
    return;

  /* Select the Joystick Report Endpoint */
  Endpoint_SelectEndpoint(JOYSTICK_EPADDR_IN);

  /* Check to see if the host is ready for another packet */
  if (Endpoint_IsINReady()) {
    bit_write(bit_check(controller.buttons, UP),
              gamepad_state.digital_buttons_1, XBOX_DPAD_UP);
    bit_write(bit_check(controller.buttons, DOWN),
              gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN);
    bit_write(bit_check(controller.buttons, LEFT),
              gamepad_state.digital_buttons_1, XBOX_DPAD_LEFT);
    bit_write(bit_check(controller.buttons, RIGHT),
              gamepad_state.digital_buttons_1, XBOX_DPAD_RIGHT);
    bit_write(bit_check(controller.buttons, START),
              gamepad_state.digital_buttons_1, XBOX_START);
    bit_write(bit_check(controller.buttons, SELECT),
              gamepad_state.digital_buttons_1, XBOX_BACK);
    bit_write(bit_check(controller.buttons, GREEN),
              gamepad_state.digital_buttons_2, XBOX_A);
    bit_write(bit_check(controller.buttons, RED),
              gamepad_state.digital_buttons_2, XBOX_B);
    bit_write(bit_check(controller.buttons, YELLOW),
              gamepad_state.digital_buttons_2, XBOX_Y);
    bit_write(bit_check(controller.buttons, BLUE),
              gamepad_state.digital_buttons_2, XBOX_X);
    bit_write(bit_check(controller.buttons, ORANGE),
              gamepad_state.digital_buttons_2, XBOX_LB);
    bit_write(bit_check(controller.buttons, RB),
              gamepad_state.digital_buttons_2, XBOX_RB);
    bit_write(bit_check(controller.buttons, HOME),
              gamepad_state.digital_buttons_2, XBOX_HOME);

    gamepad_state.l_x = controller.l_x;
    gamepad_state.l_y = controller.l_y;
    gamepad_state.r_x = controller.r_x;
    gamepad_state.r_y = controller.r_y;
    gamepad_state.lt = controller.lt;
    gamepad_state.rt = controller.rt;
    /* Write Joystick Report Data */
    Endpoint_Write_Stream_LE(&gamepad_state, 20, NULL);

    /* Finalize the stream transfer to send the last packet */
    Endpoint_ClearIN();
  }
}
/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */

/** Device descriptor structure. This descriptor, located in FLASH memory,
 * describes the overall device characteristics, including the supported USB
 * version, control endpoint size and the number of device configurations. The
 * descriptor is read out by the USB host when the enumeration process begins.
 */

const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
  Header : {Size : sizeof(USB_Descriptor_Device_t), Type : DTYPE_Device},

  USBSpecification : VERSION_BCD(2, 0, 0),
  Class : 0xFF,
  SubClass : 0xFF,
  Protocol : 0xFF,
  Endpoint0Size : 0x40,
  VendorID : 0x1209,
  ProductID : 0x2882,
  ReleaseNumber : 0x3122,

  ManufacturerStrIndex : 0x01,
  ProductStrIndex : 0x02,
  SerialNumStrIndex : 0x03,

  NumberOfConfigurations : 0x01
};

const USB_OSDescriptor_t PROGMEM OSDescriptorString = {
  Header : {Size : sizeof(USB_OSDescriptor_t), Type : DTYPE_String},
  Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
  VendorCode : REQ_GetOSFeatureDescriptor,
  Reserved : 0
};

uint16_t XInputOutput::get_descriptor(const uint8_t DescriptorType,
                                      const uint8_t DescriptorNumber,
                                      const void **const DescriptorAddress,
                                      uint8_t *const DescriptorMemorySpace) {
  uint16_t Size = NO_DESCRIPTOR;
  const void *Address = NULL;
  uint8_t MemorySpace = MEMSPACE_FLASH;
  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
    MemorySpace = MEMSPACE_RAM;
    break;
  case DTYPE_String:
    switch (DescriptorNumber) {
    case 0xEE:
      /* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
      Address = &OSDescriptorString;
      Size = pgm_read_byte(&OSDescriptorString.Header.Size);
      break;
    }
    break;
  }
  *DescriptorMemorySpace = MemorySpace;
  *DescriptorAddress = Address;
  return Size;
}

const USB_OSCompatibleIDDescriptor_t PROGMEM DevCompatIDs = {
    sizeof(USB_OSCompatibleIDDescriptor_t),
    0x0100,
    EXTENDED_COMPAT_ID_DESCRIPTOR,
    1,
    {},
    WCID_IF_NUMBER,
    0x04,
    "XUSB10"};

uint16_t USB_GetOSFeatureDescriptor(const uint8_t InterfaceNumber,
                                    const uint8_t wIndex,
                                    const uint8_t Recipient,
                                    const void **const DescriptorAddress) {
  const void *Address = NULL;
  uint16_t Size = NO_DESCRIPTOR;

  /* Check if an OS Feature Descriptor is being requested */
  switch (wIndex) {
  case EXTENDED_COMPAT_ID_DESCRIPTOR:
    // Ignore InterfaceNumber as this is a Device Request
    if (Recipient == REQREC_DEVICE) {
      Address = &DevCompatIDs;
      Size = DevCompatIDs.TotalLength;
    }
    break;
  }

  *DescriptorAddress = Address;
  return Size;
}