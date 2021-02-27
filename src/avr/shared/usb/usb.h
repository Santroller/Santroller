#include "config/defines.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/serial_handler.h"
void deviceControlRequest(void) {
  if (!(Endpoint_IsSETUPReceived())) return;
  const void *buffer = NULL;
  uint8_t len = 0;
  if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
      USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
    processHIDReadFeatureReport(USB_ControlRequest.wValue);
  } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    processHIDWriteFeatureReportControl(USB_ControlRequest.wValue,
                                        USB_ControlRequest.wLength);
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             USB_ControlRequest.wIndex == INTERFACE_ID_XInput &&
             USB_ControlRequest.wValue == 0x0000) {
    len = sizeof(capabilities1);
    buffer = &capabilities1;
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             USB_ControlRequest.wIndex == EXTENDED_PROPERTIES_DESCRIPTOR &&
             USB_ControlRequest.wValue == INTERFACE_ID_Config) {
    len = ExtendedIDs.TotalLength;
    buffer = &ExtendedIDs;
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE) &&
             USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    len = DevCompatIDs.TotalLength;
    buffer = &DevCompatIDs;
  } else if (USB_ControlRequest.bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    buffer = &DevCompatIDs;
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) &&
             USB_ControlRequest.wIndex == 0x00 &&
             USB_ControlRequest.wValue == 0x0000) {
    len = sizeof(ID);
    buffer = &ID;
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             USB_ControlRequest.wIndex == INTERFACE_ID_XInput &&
             USB_ControlRequest.wValue == 0x0100) {
    len = sizeof(capabilities2);
    buffer = &capabilities2;
  } 
  if (buffer) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_PStream_LE(buffer, len);
    Endpoint_ClearStatusStage();
  }
}
uint8_t write_endpoint_mods(const void *const Buffer, uint16_t Length,
                            uint8_t *mods, uint8_t modCount) {
  Endpoint_ClearSETUP();
  bool LastPacketFull = false;
  uint8_t current = 0;

  if (Length > USB_ControlRequest.wLength)
    Length = USB_ControlRequest.wLength;
  else if (!(Length))
    Endpoint_ClearIN();

  while (true) {
    uint8_t USB_DeviceState_LCL = USB_DeviceState;

    if (USB_DeviceState_LCL == DEVICE_STATE_Unattached)
      return ENDPOINT_RWCSTREAM_DeviceDisconnected;
    else if (USB_DeviceState_LCL == DEVICE_STATE_Suspended)
      return ENDPOINT_RWCSTREAM_BusSuspended;
    else if (Endpoint_IsSETUPReceived())
      return ENDPOINT_RWCSTREAM_HostAborted;
    else if (Endpoint_IsOUTReceived())
      break;
    if (Length || LastPacketFull) {
      if (Endpoint_IsINReady()) {
        uint16_t BytesInEndpoint = Endpoint_BytesInEndpoint();

        while (Length && (BytesInEndpoint < USB_Device_ControlEndpointSize)) {
          uint8_t bytes = 1;
          for (uint8_t i = 0; i < modCount; i += 3) {
            if (current == mods[i]) {
              bytes = 2;
              Endpoint_Write_8(mods[i + 1]);
              Endpoint_Write_8(mods[i + 2]);
            }
          }
          if (bytes == 1) { Endpoint_Write_8(pgm_read_byte(Buffer + current)); }
          // We need to skip over 2 bytes if we find a block to modify, as each
          // mod block overwrites two bytes
          Length -= bytes;
          BytesInEndpoint += bytes;
          current += bytes;
        }

        LastPacketFull = (BytesInEndpoint == USB_Device_ControlEndpointSize);
        Endpoint_ClearIN();
      }
    }
  }

  // while (!(Endpoint_IsOUTReceived())) {
  //   uint8_t USB_DeviceState_LCL = USB_DeviceState;

  //   if (USB_DeviceState_LCL == DEVICE_STATE_Unattached)
  //     return ENDPOINT_RWCSTREAM_DeviceDisconnected;
  //   else if (USB_DeviceState_LCL == DEVICE_STATE_Suspended)
  //     return ENDPOINT_RWCSTREAM_BusSuspended;
  //   else if (Endpoint_IsSETUPReceived())
  //     return ENDPOINT_RWCSTREAM_HostAborted;
  // }
  Endpoint_ClearOUT();
  return ENDPOINT_RWCSTREAM_NoError;
}
/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const descriptorAddress) {
  const uint8_t descriptorType = (wValue >> 8);
  const uint8_t descriptorNumber = (wValue & 0xFF);
  uint16_t size = NO_DESCRIPTOR;
  const void *address = NULL;
  uint8_t mods[6] = {};
  switch (descriptorType) {
  case DTYPE_Device:
    address = &deviceDescriptor;
    size = deviceDescriptor.Header.Size;
    if (deviceType >= SWITCH_GAMEPAD && deviceType < MOUSE) {
      uint8_t offs = (deviceType - SWITCH_GAMEPAD) * 2;
      mods[0] = offsetof(USB_Descriptor_Device_t, VendorID);
      mods[1] = pgm_read_byte(((uint8_t *)vid) + offs);
      mods[2] = pgm_read_byte(((uint8_t *)vid) + offs + 1);
      mods[3] = offsetof(USB_Descriptor_Device_t, ProductID);
      mods[4] = pgm_read_byte(((uint8_t *)pid) + offs);
      mods[5] = pgm_read_byte(((uint8_t *)pid) + offs + 1);
      write_endpoint_mods(address, size, mods, sizeof(mods));
    } else {
      write_endpoint_mods(address, size, mods, 0);
    }
    return NO_DESCRIPTOR;
  case DTYPE_Configuration:
    address = &ConfigurationDescriptor;
    size = ConfigurationDescriptor.Config.TotalConfigurationSize;
    mods[0] = offsetof(USB_Descriptor_Configuration_t, XInputReserved.subtype);
    mods[1] = deviceType;
    mods[2] = 0x25;
    // Interestinlgy, we don't have do do this currently as we somehow have made
    // it so that both descriptors are the same length. mods[3] =
    //     offsetof(USB_Descriptor_Configuration_t,
    //     HIDDescriptor.HIDReportLength);
    // mods[4] = 0;
    // mods[5] = sizeof(ps3_report_descriptor);
    // if (deviceType >= SWITCH_GAMEPAD) {
    write_endpoint_mods(address, size, mods, 3);
    // } else {
    //   write_endpoint_mods(address, size, mods, 3);
    // }
#ifdef MULTI_ADAPTOR
// TODO: if we ever implement this stuff, this needs to be implemented again.
// conf->XInputReserved2.subtype = XINPUT_ARCADE_PAD;
// conf->XInputReserved3.subtype = XINPUT_DANCE_PAD;
// conf->XInputReserved4.subtype = REAL_DRUM_SUBTYPE;
#endif
    return NO_DESCRIPTOR;
    break;
  case HID_DTYPE_Report:
    if (deviceType <= KEYBOARD_ROCK_BAND_DRUMS) {
      address = kbd_report_descriptor;
    } else {
      address = ps3_report_descriptor;
    }
    size = sizeof(kbd_report_descriptor);
    write_endpoint_mods(address, size, NULL, 0);
    return 0;
  case DTYPE_String:
    if (descriptorNumber <= 3) {
      address = (void *)pgm_read_word(descriptorStrings + descriptorNumber);
    } else if (descriptorNumber == 0xEE) {
      address = &OSDescriptorString;
    } else {
      break;
    }
    size =
        pgm_read_byte(address + offsetof(USB_StdDescriptor_String_t, bLength));
    break;
  }
  *descriptorAddress = address;

  return size;
}